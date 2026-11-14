#include "wiegand.h"
#include <furi.h>
#include <furi_hal.h>

unsigned long WIEGAND::_cardTempHigh = 0;
unsigned long WIEGAND::_cardTemp = 0;
unsigned long WIEGAND::_lastWiegand = 0;
unsigned long WIEGAND::_code = 0;
unsigned long WIEGAND::_codeHigh = 0;
int WIEGAND::_bitCount = 0;
int WIEGAND::_wiegandType = 0;

constexpr uint32_t clocks_in_ms = 64 * 1000;
const GpioPin* const pinD0 = &gpio_ext_pa4;
const GpioPin* const pinD1 = &gpio_ext_pa7;

WIEGAND::WIEGAND() {
}

unsigned long WIEGAND::getCode() {
    return _code;
}

unsigned long WIEGAND::getCodeHigh() {
    return _codeHigh;
}

int WIEGAND::getWiegandType() {
    return _wiegandType;
}

bool WIEGAND::available() {
    bool ret;
    FURI_CRITICAL_ENTER();
    ret = DoWiegandConversion();
    FURI_CRITICAL_EXIT();
    return ret;
}

static void input_isr_d0(void* _ctx) {
    WIEGAND* _this = static_cast<WIEGAND*>(_ctx);
    _this->ReadD0();
}

static void input_isr_d1(void* _ctx) {
    WIEGAND* _this = static_cast<WIEGAND*>(_ctx);
    _this->ReadD1();
}

void WIEGAND::begin() {
    _lastWiegand = 0;
    _cardTempHigh = 0;
    _cardTemp = 0;
    _code = 0;
    _wiegandType = 0;
    _bitCount = 0;

    furi_hal_gpio_init_simple(pinD0, GpioModeInterruptFall);
    furi_hal_gpio_init_simple(pinD1, GpioModeInterruptFall);

    furi_hal_gpio_add_int_callback(pinD0, input_isr_d0, this);
    furi_hal_gpio_add_int_callback(pinD1, input_isr_d1, this);
}

void WIEGAND::end() {
    furi_hal_gpio_remove_int_callback(pinD0);
    furi_hal_gpio_remove_int_callback(pinD1);

    furi_hal_gpio_init_simple(pinD0, GpioModeAnalog);
    furi_hal_gpio_init_simple(pinD1, GpioModeAnalog);
}

void WIEGAND::ReadD0() {
    _bitCount++;
    if(_bitCount > 31) // If bit count more than 31, process high bits
    {
        _cardTempHigh |= ((0x80000000 & _cardTemp) >> 31);
        _cardTempHigh <<= 1;
        _cardTemp <<= 1;
    } else {
        _cardTemp <<= 1; // D0 represent binary 0, so just left shift card data
    }
    _lastWiegand = DWT->CYCCNT;
}

void WIEGAND::ReadD1() {
    _bitCount++;
    if(_bitCount > 31) // If bit count more than 31, process high bits
    {
        _cardTempHigh |= ((0x80000000 & _cardTemp) >> 31);
        _cardTempHigh <<= 1;
        _cardTemp |= 1;
        _cardTemp <<= 1;
    } else {
        _cardTemp |= 1; /* D1 représente bit 1 */
        _cardTemp <<= 1; // left shift card data
    }
    _lastWiegand = DWT->CYCCNT;
}

unsigned long WIEGAND::GetCardId(unsigned long* codehigh, unsigned long* codelow, char bitlength) {
    if(bitlength == 26) // EM tag
        return (*codelow & 0x1FFFFFE) >> 1;

    if(bitlength == 24) return (*codelow & 0x7FFFFE) >> 1;

    if(bitlength == 34) // Mifare
    {
        *codehigh = *codehigh & 0x03; /* Seuls deux LSB de codehigh requis */
        *codehigh <<= 30;
        *codelow >>= 1;
        return *codehigh | *codelow;
    }

    if(bitlength == 32) {
        return (*codelow & 0x7FFFFFFE) >> 1;
    }

    return *codelow; /* EM ou Mifare sans bits de parité */
}

char translateEnterEscapeKeyPress(char originalKeyPress) {
    switch(originalKeyPress) {
    case 0x0b:
        return 0x0d;

    case 0x0a:
        return 0x1b;

    default:
        return originalKeyPress;
    }
}

bool WIEGAND::DoWiegandConversion() {
    unsigned long cardID;
    unsigned long sysTick = DWT->CYCCNT;

    if((sysTick - _lastWiegand) >
       (25 * clocks_in_ms)) /* Fin réception après 25 ms sans signal */
    {
        if((_bitCount == 24) || (_bitCount == 26) || (_bitCount == 32) || (_bitCount == 34) ||
           (_bitCount == 37) || (_bitCount == 40) || (_bitCount == 8) ||
           (_bitCount ==
            4)) /* Bits : clavier 4/8 ; Wiegand26 24/26 ; Wiegand34 32/34 */
        {
            _codeHigh = 0;
            /* Décalage droit compense décalage anticipé de l’ISR */
            _cardTemp >>= 1;
            /* Au-delà de 32 bits, ajuster bits hauts */
            if(_bitCount > 32) _cardTempHigh >>= 1;

            if(_bitCount == 8)
            {
                /* Wiegand 8 bits : nibble haut complément du bas */
                /* Touche 1 : E1 ; nibble haut 1110, bas 0001 */
                char highNibble = (_cardTemp & 0xf0) >> 4;
                char lowNibble = (_cardTemp & 0x0f);
                _wiegandType = _bitCount;
                _bitCount = 0;
                _cardTemp = 0;
                _cardTempHigh = 0;

                if(lowNibble ==
                   (~highNibble & 0x0f)) /* Vérifier nibbles complémentaires */
                {
                    _code = (int)translateEnterEscapeKeyPress(lowNibble);
                    return true;
                } else {
                    _lastWiegand = sysTick;
                    return false;
                }

                /* TODO FL-3490: gérer échec validation */
            } else if(4 == _bitCount) {
                /* Wiegand quatre bits sans contrôle intégrité */

                _code = (int)translateEnterEscapeKeyPress(_cardTemp & 0x0000000F);

                _wiegandType = _bitCount;
                _bitCount = 0;
                _cardTemp = 0;
                _cardTempHigh = 0;

                return true;
            } else if(40 == _bitCount) {
                _cardTempHigh >>= 1;

                _code = _cardTemp;
                _codeHigh = _cardTempHigh;

                _wiegandType = _bitCount;
                _bitCount = 0;
                _cardTemp = 0;
                _cardTempHigh = 0;

                return true;
            } else {

                cardID = GetCardId(&_cardTempHigh, &_cardTemp, _bitCount);
                _wiegandType = _bitCount;
                _bitCount = 0;
                _cardTemp = 0;
                _cardTempHigh = 0;
                _code = cardID;
                return true;
            }
        } else {
            /* Après 25 ms : tailles hors 8/26/34 considérées bruit */
            _lastWiegand = sysTick;
            _bitCount = 0;
            _cardTemp = 0;
            _cardTempHigh = 0;
            return false;
        }
    } else
        return false;
}
