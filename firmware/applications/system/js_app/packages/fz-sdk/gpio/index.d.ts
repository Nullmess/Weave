// Importer event_loop avant GPIO




























import type { Contract } from "../event_loop";




export interface Mode {
    direction: "in" | "out";
    outMode?: "push_pull" | "open_drain";
    inMode?: "analog" | "plain_digital" | "interrupt" | "event";
    edge?: "rising" | "falling" | "both";
    pull?: "up" | "down";
}




export interface Pin {
    // Configuration pin répétable




    init(mode: Mode): void;
    // Pin direction out requise





    write(value: boolean): void;
    // Pin direction in hors analog requise




    read(): boolean;
    // Pin in/analog : voltage en mV




    readAnalog(): number;
    // Event IRQ selon configuration init




    interrupt(): Contract;
    // PWM PA4/PA7 seulement ; SDK0.2 gpio-pwm, baseline1.0






    isPwmSupported(): boolean;
    // PWM out push_pull ; fréquence Hz, duty % ; erreur pin incompatible








    pwmWrite(freq: number, duty: number): void;
    // Erreur si pin sans PWM





    isPwmRunning(): boolean;
    // Stop PWM sans restauration config précédente ; erreur pin incompatible





    pwmStop(): void;
}

// Pin par nom ou numéro ; docs.flipper.net/zero/gpio-and-modules





export function get(pin: string | number): Pin;
