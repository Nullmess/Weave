/* DS18B20 : mesures threadées ; datasheet Analog Devices */

#include <gui/gui.h>
#include <gui/view_port.h>

#include <core/thread.h>
#include <core/kernel.h>

#include <locale/locale.h>

#include <one_wire/maxim_crc.h>
#include <one_wire/one_wire_host.h>

#include <power/power_service/power.h>

#define UPDATE_PERIOD_MS 1000UL
#define TEXT_STORE_SIZE  64U

#define DS18B20_CMD_SKIP_ROM        0xccU
#define DS18B20_CMD_CONVERT         0x44U
#define DS18B20_CMD_READ_SCRATCHPAD 0xbeU

#define DS18B20_CFG_RESOLUTION_POS  5U
#define DS18B20_CFG_RESOLUTION_MASK 0x03U
#define DS18B20_DECIMAL_PART_MASK   0x0fU

#define DS18B20_SIGN_MASK 0xf0U

/* GPIO externes PC0/PC1/PC3/PB2/PB3/PA4/PA6/PA7 ou gpio_ibutton */

#define THERMO_GPIO_PIN (gpio_ibutton)

typedef enum {
    ReaderThreadFlagExit = 1,
} ReaderThreadFlag;

typedef union {
    struct {
        uint8_t temp_lsb; /* LSB température */
        uint8_t temp_msb; /* MSB température */
        uint8_t user_alarm_high; /* Registre utilisateur 1 : alarme température haute */
        uint8_t user_alarm_low; /* Registre utilisateur 2 : alarme température basse */
        uint8_t config;
        uint8_t reserved[3]; /* Inutilisé */
        uint8_t crc; /* CRC de détection erreurs */
    } fields;
    uint8_t bytes[9];
} DS18B20Scratchpad;

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriThread* reader_thread;
    FuriMessageQueue* event_queue;
    OneWireHost* onewire;
    Power* power;
    float temp_celsius;
    bool has_device;
} ExampleThermoContext;

static void example_thermo_request_temperature(ExampleThermoContext* context) {
    OneWireHost* onewire = context->onewire;

    /* Transactions 1-Wire en section critique non interrompue */
    FURI_CRITICAL_ENTER();

    bool success = false;
    do {
        /* Reset initial ; true si pulse présence détecté */
        if(!onewire_host_reset(onewire)) break;
        /* ROM obligatoire après reset ; Skip ROM si device unique */
        onewire_host_write(onewire, DS18B20_CMD_SKIP_ROM);
        /* Commande mesure après opération ROM */
        onewire_host_write(onewire, DS18B20_CMD_CONVERT);

        success = true;
    } while(false);

    context->has_device = success;

    FURI_CRITICAL_EXIT();
}

static void example_thermo_read_temperature(ExampleThermoContext* context) {
    /* Aucune lecture si device non détecté */
    if(!context->has_device) {
        return;
    }

    OneWireHost* onewire = context->onewire;

    /* Transactions 1-Wire en section critique non interrompue */
    FURI_CRITICAL_ENTER();

    bool success = false;

    do {
        DS18B20Scratchpad buf;

        /* Dix tentatives de lecture maximum */
        size_t attempts_left = 10;
        do {
            /* Reset initial ; true si pulse présence détecté */
            if(!onewire_host_reset(onewire)) continue;

            /* ROM obligatoire après reset ; Skip ROM si device unique */
            onewire_host_write(onewire, DS18B20_CMD_SKIP_ROM);

            /* Read Scratchpad après ROM, avant lecture du buffer interne */
            onewire_host_write(onewire, DS18B20_CMD_READ_SCRATCHPAD);

            /* Lecture de 9 bytes */
            onewire_host_read_bytes(onewire, buf.bytes, sizeof(buf.bytes));

            const uint8_t crc = maxim_crc8(buf.bytes, sizeof(buf.bytes) - 1, MAXIM_CRC8_INIT);

            if(crc == buf.fields.crc) break;

        } while(--attempts_left);

        if(attempts_left == 0) break;

        /* Résolution depuis registre configuration, DS18B20 p.9 */
        const uint8_t resolution_mode = (buf.fields.config >> DS18B20_CFG_RESOLUTION_POS) &
                                        DS18B20_CFG_RESOLUTION_MASK;

        /* Masque bits fractionnels indéfinis, DS18B20 p.6 */
        const uint8_t decimal_mask =
            (DS18B20_DECIMAL_PART_MASK << (DS18B20_CFG_RESOLUTION_MASK - resolution_mode)) &
            DS18B20_DECIMAL_PART_MASK;

        /* Parties entière/fractionnelle, DS18B20 p.6 */
        const uint8_t integer_part = (buf.fields.temp_msb << 4U) | (buf.fields.temp_lsb >> 4U);
        const uint8_t decimal_part = buf.fields.temp_lsb & decimal_mask;

        /* Signe température, DS18B20 p.6 */
        const bool is_negative = (buf.fields.temp_msb & DS18B20_SIGN_MASK) != 0;

        const float temp_celsius_abs = integer_part + decimal_part / 16.f;

        context->temp_celsius = is_negative ? -temp_celsius_abs : temp_celsius_abs;

        success = true;
    } while(false);

    context->has_device = success;

    FURI_CRITICAL_EXIT();
}

/* Mesures périodiques dans un thread séparé */
static int32_t example_thermo_reader_thread_callback(void* ctx) {
    ExampleThermoContext* context = ctx;

    for(;;) {
        /* Mesure température : jusqu’à 750 ms */
        example_thermo_request_temperature(context);

        /* Attente mesure ou signal de sortie */
        const uint32_t flags =
            furi_thread_flags_wait(ReaderThreadFlagExit, FuriFlagWaitAny, UPDATE_PERIOD_MS);

        if(flags != (unsigned)FuriFlagErrorTimeout) break;

        example_thermo_read_temperature(context);
    }

    return 0;
}

/* Écran entièrement redessiné à chaque appel */
static void example_thermo_draw_callback(Canvas* canvas, void* ctx) {
    ExampleThermoContext* context = ctx;
    char text_store[TEXT_STORE_SIZE];
    const size_t middle_x = canvas_width(canvas) / 2U;

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, middle_x, 12, AlignCenter, AlignBottom, "Thermometer Demo");
    canvas_draw_line(canvas, 0, 16, 128, 16);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, middle_x, 30, AlignCenter, AlignBottom, "Connect thermometer");

    snprintf(
        text_store,
        TEXT_STORE_SIZE,
        "to GPIO pin %ld",
        furi_hal_resources_get_ext_pin_number(&THERMO_GPIO_PIN));
    canvas_draw_str_aligned(canvas, middle_x, 42, AlignCenter, AlignBottom, text_store);

    canvas_set_font(canvas, FontKeyboard);

    if(context->has_device) {
        float temp;
        char temp_units;

        /* Unités selon Settings/System/Units */
        switch(locale_get_measurement_unit()) {
        case LocaleMeasurementUnitsMetric:
            temp = context->temp_celsius;
            temp_units = 'C';
            break;
        case LocaleMeasurementUnitsImperial:
            temp = locale_celsius_to_fahrenheit(context->temp_celsius);
            temp_units = 'F';
            break;
        default:
            furi_crash("Illegal measurement units");
        }

        snprintf(text_store, TEXT_STORE_SIZE, "Temperature: %+.1f%c", (double)temp, temp_units);
    } else {

        strlcpy(text_store, "-- No data --", TEXT_STORE_SIZE);
    }

    canvas_draw_str_aligned(canvas, middle_x, 58, AlignCenter, AlignBottom, text_store);
}

/* Contexte GUI : envoi événement à la queue application */
static void example_thermo_input_callback(InputEvent* event, void* ctx) {
    ExampleThermoContext* context = ctx;
    furi_message_queue_put(context->event_queue, event, FuriWaitForever);
}

static void example_thermo_run(ExampleThermoContext* context) {
    /* Alimentation activée sur pins externes */
    power_enable_otg(context->power, true);

    /* Configuration hardware en mode host */
    onewire_host_start(context->onewire);

    /* Thread lecteur en arrière-plan */
    furi_thread_start(context->reader_thread);

    for(bool is_running = true; is_running;) {
        InputEvent event;
        /* Input events envoyés par callback depuis le thread GUI */
        const FuriStatus status =
            furi_message_queue_get(context->event_queue, &event, FuriWaitForever);

        /* Seuls les appuis courts sont traités */
        if((status != FuriStatusOk) || (event.type != InputTypeShort)) {
            continue;
        }

        if(event.key == InputKeyBack) {
            is_running = false;
        }
    }

    furi_thread_flags_set(furi_thread_get_id(context->reader_thread), ReaderThreadFlagExit);

    furi_thread_join(context->reader_thread);

    onewire_host_stop(context->onewire);

    /* Alimentation désactivée sur pins externes */
    power_enable_otg(context->power, false);
}

static ExampleThermoContext* example_thermo_context_alloc(void) {
    ExampleThermoContext* context = malloc(sizeof(ExampleThermoContext));

    context->view_port = view_port_alloc();
    view_port_draw_callback_set(context->view_port, example_thermo_draw_callback, context);
    view_port_input_callback_set(context->view_port, example_thermo_input_callback, context);

    context->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    context->reader_thread = furi_thread_alloc();
    furi_thread_set_stack_size(context->reader_thread, 1024U);
    furi_thread_set_context(context->reader_thread, context);
    furi_thread_set_callback(context->reader_thread, example_thermo_reader_thread_callback);

    context->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(context->gui, context->view_port, GuiLayerFullscreen);

    context->onewire = onewire_host_alloc(&THERMO_GPIO_PIN);

    context->power = furi_record_open(RECORD_POWER);

    return context;
}

static void example_thermo_context_free(ExampleThermoContext* context) {
    view_port_enabled_set(context->view_port, false);
    gui_remove_view_port(context->gui, context->view_port);

    onewire_host_free(context->onewire);
    furi_thread_free(context->reader_thread);
    furi_message_queue_free(context->event_queue);
    view_port_free(context->view_port);

    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_POWER);
}

int32_t example_thermo_main(void* p) {
    UNUSED(p);

    ExampleThermoContext* context = example_thermo_context_alloc();

    example_thermo_run(context);

    example_thermo_context_free(context);

    return 0;
}
