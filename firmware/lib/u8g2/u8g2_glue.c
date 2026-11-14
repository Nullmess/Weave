#include "u8g2_glue.h"

#include <furi_hal.h>

#define CONTRAST_ERC 32
#define CONTRAST_MGG 28

uint8_t u8g2_gpio_and_delay_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    UNUSED(u8x8);
    UNUSED(arg_ptr);
    switch(msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        /* HAL initialise déjà cette partie */
        break;
    case U8X8_MSG_DELAY_MILLI:
        furi_delay_ms(arg_int);
        break;
    case U8X8_MSG_DELAY_10MICRO:
        furi_delay_us(10);
        break;
    case U8X8_MSG_DELAY_100NANO:
        asm("nop");
        break;
    case U8X8_MSG_GPIO_RESET:
        furi_hal_gpio_write(&gpio_display_rst_n, arg_int);
        break;
    default:
        return 0;
    }

    return 1;
}

uint8_t u8x8_hw_spi_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    UNUSED(u8x8);
    switch(msg) {
    case U8X8_MSG_BYTE_SEND:
        furi_hal_spi_bus_tx(&furi_hal_spi_bus_handle_display, (uint8_t*)arg_ptr, arg_int, 10000);
        break;
    case U8X8_MSG_BYTE_SET_DC:
        furi_hal_gpio_write(&gpio_display_di, arg_int);
        break;
    case U8X8_MSG_BYTE_INIT:
        break;
    case U8X8_MSG_BYTE_START_TRANSFER:
        furi_hal_spi_acquire(&furi_hal_spi_bus_handle_display);
        break;
    case U8X8_MSG_BYTE_END_TRANSFER:
        furi_hal_spi_release(&furi_hal_spi_bus_handle_display);
        break;
    default:
        return 0;
    }

    return 1;
}

#define ST756X_CMD_ON_OFF           0b10101110 /* 0:0 display ON/OFF : dernier bit */
#define ST756X_CMD_SET_LINE         0b01000000 /* 0:0 ligne initiale : six derniers bits */
#define ST756X_CMD_SET_PAGE         0b10110000 /* 0:0 adresse page : quatre derniers bits */
#define ST756X_CMD_SET_COLUMN_MSB   0b00010000 /* 0:0 colonne MSB : quatre derniers bits */
#define ST756X_CMD_SET_COLUMN_LSB   0b00000000 /* 0:0 colonne LSB : quatre derniers bits */
#define ST756X_CMD_SEG_DIRECTION    0b10100000 /* 0:0 inversion SEG : dernier bit */
#define ST756X_CMD_INVERSE_DISPLAY  0b10100110 /* 0:0 inversion affichage : dernier bit */
#define ST756X_CMD_ALL_PIXEL_ON     0b10100100 /* 0:0 tous pixels actifs : dernier bit */
#define ST756X_CMD_BIAS_SELECT      0b10100010 /* 0:0 bias 1/9=0, 1/7=1 : dernier bit */
#define ST756X_CMD_R_M_W            0b11100000 /* 0:0 mode Read Modify Write : lecture+0, écriture+1 */
#define ST756X_CMD_END              0b11101110 /* 0:0 sortie Read Modify Write */
#define ST756X_CMD_RESET            0b11100010 /* 0:0 reset logiciel */
#define ST756X_CMD_COM_DIRECTION    0b11000000 /* 0:0 inversion COM : +0b1000 */
#define ST756X_CMD_POWER_CONTROL    0b00101000 /* 0:0 power : trois derniers bits VB:VR:VF */
#define ST756X_CMD_REGULATION_RATIO 0b00100000 /* 0:0 ratio résistance : trois derniers bits */
#define ST756X_CMD_SET_EV           0b10000001 /* 0:0 volume électronique : cinq bits octet suivant */
#define ST756X_CMD_SET_BOOSTER \
    0b11111000 /* 0:0 booster 4X=0/5X=1 : dernier bit octet suivant */
#define ST756X_CMD_NOP 0b11100011 /* 0:0 sans opération */

static const uint8_t u8x8_d_st756x_powersave0_seq[] = {
    U8X8_START_TRANSFER(), /* Activation chip ; délai intégré au start transfer */
    U8X8_C(ST756X_CMD_ALL_PIXEL_ON | 0b0),
    U8X8_C(ST756X_CMD_ON_OFF | 0b1),
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static const uint8_t u8x8_d_st756x_powersave1_seq[] = {
    U8X8_START_TRANSFER(), /* Activation chip ; délai intégré au start transfer */
    U8X8_C(ST756X_CMD_ON_OFF | 0b0),
    U8X8_C(ST756X_CMD_ALL_PIXEL_ON | 0b1),
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static const uint8_t u8x8_d_st756x_flip0_seq[] = {
    U8X8_START_TRANSFER(), /* Activation chip ; délai intégré au start transfer */
    U8X8_C(0x0a1), /* segment remap a0/a1*/
    U8X8_C(0x0c0), /* c0 scan normal ; c8 inversé */
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static const uint8_t u8x8_d_st756x_flip1_seq[] = {
    U8X8_START_TRANSFER(), /* Activation chip ; délai intégré au start transfer */
    U8X8_C(0x0a0), /* segment remap a0/a1*/
    U8X8_C(0x0c8), /* c0 scan normal ; c8 inversé */
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static const u8x8_display_info_t u8x8_st756x_128x64_display_info = {
    .chip_enable_level = 0,
    .chip_disable_level = 1,
    .post_chip_enable_wait_ns = 150, /* st7565 datasheet, table 26, tcsh */
    .pre_chip_disable_wait_ns = 50, /* st7565 datasheet, table 26, tcss */
    .reset_pulse_width_ms = 1,
    .post_reset_wait_ms = 1,
    .sda_setup_time_ns = 50, /* st7565 datasheet, table 26, tsds */
    .sck_pulse_width_ns =
        120, /* Demi-cycle 100 ns ; AVR <70 : 8 MHz, sinon 4 MHz */
    .sck_clock_hz =
        4000000UL, /* Arduino >=1.6.0 : SPI en Hz, 1e9/sck_pulse_width_ns */
    .spi_mode = 0, /* Actif haut, front montant */
    .i2c_bus_clock_100kHz = 4,
    .data_setup_time_ns = 40, /* st7565 datasheet, table 24, tds8 */
    .write_pulse_width_ns = 80, /* st7565 datasheet, table 24, tcclw */
    .tile_width = 16, /* Largeur : 16*8=128 pixels */
    .tile_height = 8,
    .default_x_offset = 0,
    .flipmode_x_offset = 4,
    .pixel_width = 128,
    .pixel_height = 64};

uint8_t u8x8_d_st756x_common(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    uint8_t x, c;
    uint8_t* ptr;

    switch(msg) {
    case U8X8_MSG_DISPLAY_DRAW_TILE:
        u8x8_cad_StartTransfer(u8x8);

        x = ((u8x8_tile_t*)arg_ptr)->x_pos;
        x *= 8;
        x += u8x8->x_offset;
        u8x8_cad_SendCmd(u8x8, 0x010 | (x >> 4));
        u8x8_cad_SendCmd(u8x8, 0x000 | (x & 15));
        u8x8_cad_SendCmd(u8x8, 0x0b0 | (((u8x8_tile_t*)arg_ptr)->y_pos));

        c = ((u8x8_tile_t*)arg_ptr)->cnt;
        c *= 8;
        ptr = ((u8x8_tile_t*)arg_ptr)->tile_ptr;
        /* ST7565 : écriture hors écran interdite, attention flip mode */

        if(c + x > 132u) {
            c = 132u;
            c -= x;
        }

        do {
            u8x8_cad_SendData(
                u8x8, c, ptr); /* SendData limité à 255 octets */
            arg_int--;
        } while(arg_int > 0);

        u8x8_cad_EndTransfer(u8x8);
        break;
    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
        if(arg_int == 0)
            u8x8_cad_SendSequence(u8x8, u8x8_d_st756x_powersave0_seq);
        else
            u8x8_cad_SendSequence(u8x8, u8x8_d_st756x_powersave1_seq);
        break;
#ifdef U8X8_WITH_SET_CONTRAST
    case U8X8_MSG_DISPLAY_SET_CONTRAST:
        u8x8_cad_StartTransfer(u8x8);
        u8x8_cad_SendCmd(u8x8, ST756X_CMD_SET_EV);
        u8x8_cad_SendArg(u8x8, arg_int >> 2); /* ST7565 : 0..63 */
        u8x8_cad_EndTransfer(u8x8);
        break;
#endif
    default:
        return 0;
    }
    return 1;
}

void u8x8_d_st756x_init(u8x8_t* u8x8, uint8_t contrast, uint8_t regulation_ratio, bool bias) {
    contrast = contrast & 0b00111111;
    regulation_ratio = regulation_ratio & 0b111;

    u8x8_cad_StartTransfer(u8x8);

    u8x8_cad_SendCmd(u8x8, ST756X_CMD_RESET);
    /* Bias : 1/7=0b1 ; 1/9=0b0 */
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_BIAS_SELECT | bias);

    u8x8_cad_SendCmd(u8x8, ST756X_CMD_SEG_DIRECTION);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_COM_DIRECTION | 0b1000);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_SET_LINE);

    u8x8_cad_SendCmd(u8x8, ST756X_CMD_REGULATION_RATIO | regulation_ratio);

    u8x8_cad_SendCmd(u8x8, ST756X_CMD_SET_EV);
    u8x8_cad_SendArg(u8x8, contrast);

    u8x8_cad_SendCmd(u8x8, ST756X_CMD_POWER_CONTROL | 0b111);

    u8x8_cad_EndTransfer(u8x8);
}

void u8x8_d_st756x_set_contrast(u8x8_t* u8x8, int8_t contrast_offset) {
    uint8_t contrast = (furi_hal_version_get_hw_display() == FuriHalVersionDisplayMgg) ?
                           CONTRAST_MGG :
                           CONTRAST_ERC;
    contrast += contrast_offset;
    contrast = contrast & 0b00111111;

    u8x8_cad_StartTransfer(u8x8);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_SET_EV);
    u8x8_cad_SendArg(u8x8, contrast);
    u8x8_cad_EndTransfer(u8x8);
}

uint8_t u8x8_d_st756x_flipper(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {

    if(u8x8_d_st756x_common(u8x8, msg, arg_int, arg_ptr) == 0) {

        switch(msg) {
        case U8X8_MSG_DISPLAY_SETUP_MEMORY:
            u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st756x_128x64_display_info);
            break;
        case U8X8_MSG_DISPLAY_INIT:
            u8x8_d_helper_display_init(u8x8);
            FuriHalVersionDisplay display = furi_hal_version_get_hw_display();
            if(display == FuriHalVersionDisplayMgg) {
                /* MGG v0+ ST7567 : EV=32 ; bias 1/9 */
                /* RR=V0/((1-(63-EV)/162)*2.1) ; V0=10 → ~5.88, code 0b110 */

                u8x8_d_st756x_init(u8x8, CONTRAST_MGG, 0b110, false);
            } else {
                /* ERC v1 ST7565 / v2 ST7567 : EV=33 ; bias 1/9 */
                /* RR=V0/((1-(63-EV)/162)*2.1) ; V0=9.3 → ~5.47, code 0b101 */

                u8x8_d_st756x_init(u8x8, CONTRAST_ERC, 0b101, false);
            }
            break;
        case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
            if(arg_int == 0) {
                u8x8_cad_SendSequence(u8x8, u8x8_d_st756x_flip1_seq);
                u8x8->x_offset = u8x8->display_info->default_x_offset;
            } else {
                u8x8_cad_SendSequence(u8x8, u8x8_d_st756x_flip0_seq);
                u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
            }
            break;
        default:

            return 0;
        }
    }
    return 1;
}

void u8g2_Setup_st756x_flipper(
    u8g2_t* u8g2,
    const u8g2_cb_t* rotation,
    u8x8_msg_cb byte_cb,
    u8x8_msg_cb gpio_and_delay_cb) {
    uint8_t tile_buf_height;
    uint8_t* buf;
    u8g2_SetupDisplay(u8g2, u8x8_d_st756x_flipper, u8x8_cad_001, byte_cb, gpio_and_delay_cb);
    buf = u8g2_m_16_8_f(&tile_buf_height);
    u8g2_SetupBuffer(u8g2, buf, tile_buf_height, u8g2_ll_hvline_vertical_top_lsb, rotation);
}
