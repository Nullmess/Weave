export const MAX_FRAME = 1024;
export const SERVICE_UUID = '61ea0000-793d-47da-a331-4510e11e0001';
export const NOTIFY_UUID = '61ea0001-793d-47da-a331-4510e11e0001';
export const WRITE_UUID = '61ea0002-793d-47da-a331-4510e11e0001';
export type Frame = { command: string; fields: string[] };
export function encode(command: string, ...fields: string[]): Uint8Array {
    const parts = ['WV1', command, ...fields];
    if (parts.some(p => !p || /[^\x20-\x7e]/.test(p) || p.includes('|'))) throw new Error('Invalid WV1 field');
    const frame = new TextEncoder().encode(parts.join('|') + '\n');
    if (frame.length > MAX_FRAME) throw new Error('WV1 too large');
    return frame;
}
export function decode(raw: string): Frame {
    if (!raw.endsWith('\n') || raw.length > MAX_FRAME || /[^\x00-\x7f]/.test(raw)) throw new Error('Invalid WV1 frame');
    const [prefix, command, ...fields] = raw.trimEnd().split('|');
    if (prefix !== 'WV1' || !command) throw new Error('Unsupported WV1 frame');
    return { command, fields };
}
export class LineBuffer {
    private text = '';
    push(chunk: Uint8Array): Frame[] {
        this.text += new TextDecoder('utf-8', {fatal:true}).decode(chunk);
        if (this.text.length > MAX_FRAME * 4) { this.text = ''; throw new Error('WV1 buffer overflow'); }
        const frames: Frame[] = [];
        while (this.text.includes('\n')) {
            const idx = this.text.indexOf('\n');
            const line = this.text.slice(0, idx + 1);
            this.text = this.text.slice(idx + 1);
            frames.push(decode(line));
        }
        return frames;
    }
    clear():void { this.text = ''; }
}
