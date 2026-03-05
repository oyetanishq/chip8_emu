import { useEffect, useState, useRef } from "react";
import createModule, { type Chip8, type Chip8Module } from "@/lib/chip8_wasm.js";

// Standard Chip-8 Hex keyboard mapped to the left side of a QWERTY keyboard
const KEY_MAP: Record<string, number> = {
    "1": 0x1,
    "2": 0x2,
    "3": 0x3,
    "4": 0xc,
    q: 0x4,
    w: 0x5,
    e: 0x6,
    r: 0xd,
    a: 0x7,
    s: 0x8,
    d: 0x9,
    f: 0xe,
    z: 0xa,
    x: 0x0,
    c: 0xb,
    v: 0xf,
};

export default function Home() {
    const [chip8Module, setChip8Module] = useState<Chip8Module | null>(null);
    const [chip8Instance, setChip8Instance] = useState<Chip8 | null>(null);
    const [isRunning, setIsRunning] = useState(false);

    const canvasRef = useRef<HTMLCanvasElement>(null);
    const requestRef = useRef<number>(0);

    // Initialize the WASM module
    useEffect(() => {
        let chip8: Chip8;

        createModule().then((Module: Chip8Module) => {
            setChip8Module(Module);
            chip8 = new Module.Chip8();
            setChip8Instance(chip8);
        });

        return () => {
            if (chip8) chip8.delete();
        };
    }, []);

    // The Game Loop & Input Handlers
    useEffect(() => {
        if (!chip8Instance || !chip8Module || !isRunning) return;

        const canvas = canvasRef.current;
        const ctx = canvas?.getContext("2d");
        if (!canvas || !ctx) return;

        // Map JS key presses to C++ memory
        const handleKeyDown = (e: KeyboardEvent) => {
            const keyIndex = KEY_MAP[e.key.toLowerCase()];
            if (keyIndex !== undefined) {
                // Read the view directly from the instance and modify it
                const keys = chip8Instance.getKeys();
                keys[keyIndex] = 1;
            }
        };

        const handleKeyUp = (e: KeyboardEvent) => {
            const keyIndex = KEY_MAP[e.key.toLowerCase()];
            if (keyIndex !== undefined) {
                const keys = chip8Instance.getKeys();
                keys[keyIndex] = 0;
            }
        };

        window.addEventListener("keydown", handleKeyDown);
        window.addEventListener("keyup", handleKeyUp);

        // Main emulation loop
        const loop = () => {
            // Chip-8 timers update at 60Hz (which matches requestAnimationFrame),
            // but the CPU runs faster. We do ~10 cycles per frame for ~600Hz.
            for (let i = 0; i < 10; i++) {
                chip8Instance.emulate_cycle();
            }

            // Draw to canvas only if the C++ drawFlag was set to true
            if (chip8Instance.drawFlag) {
                // getGfx() directly gives you the Uint8Array
                const gfxArray = chip8Instance.getGfx();

                ctx.fillStyle = "black";
                ctx.fillRect(0, 0, canvas.width, canvas.height);
                ctx.fillStyle = "white";

                const scale = 10;

                for (let y = 0; y < 32; y++) {
                    for (let x = 0; x < 64; x++) {
                        const pixel = gfxArray[y * 64 + x];
                        if (pixel !== 0) {
                            ctx.fillRect(x * scale, y * scale, scale, scale);
                        }
                    }
                }

                chip8Instance.drawFlag = false;
            }

            requestRef.current = requestAnimationFrame(loop);
        };

        // Start loop
        requestRef.current = requestAnimationFrame(loop);

        // Cleanup event listeners and animation frame on unmount/stop
        return () => {
            window.removeEventListener("keydown", handleKeyDown);
            window.removeEventListener("keyup", handleKeyUp);
            if (requestRef.current) cancelAnimationFrame(requestRef.current);
        };
    }, [chip8Instance, chip8Module, isRunning]);

    // Load ROM from public/roms folder
    const loadRom = async (romFilename: string) => {
        if (!chip8Instance || !chip8Module) return;

        try {
            // Pause the emulator while fetching
            setIsRunning(false);
            if (requestRef.current) cancelAnimationFrame(requestRef.current);

            // Fetch binary data
            const response = await fetch(`/roms/${romFilename}`);
            if (!response.ok) throw new Error(`Could not find /roms/${romFilename}`);

            const buffer = await response.arrayBuffer();
            const uint8View = new Uint8Array(buffer);

            // Move data into C++ vector
            const vec = new chip8Module.VectorUint8();
            for (let i = 0; i < uint8View.length; i++) {
                vec.push_back(uint8View[i]);
            }

            // Load into emulator
            chip8Instance.load_rom_from_buffer(vec);

            // CRITICAL: Free the C++ vector memory immediately so we don't leak
            vec.delete();

            // Clear the canvas visually before starting
            const canvas = canvasRef.current;
            const ctx = canvas?.getContext("2d");
            if (ctx && canvas) {
                ctx.fillStyle = "black";
                ctx.fillRect(0, 0, canvas.width, canvas.height);
            }

            // Resume emulation
            setIsRunning(true);
        } catch (error) {
            console.error("ROM Load Error:", error);
            alert("Failed to load ROM. Check console for details.");
        }
    };

    if (!chip8Instance || !chip8Module) {
        return <div>Loading C++ WebAssembly...</div>;
    }

    return (
        <div>
            <h1>WASM Chip-8 Emulator</h1>

            <div>
                <p>Select a ROM to start:</p>
                {/* Ensure you have these exact filenames inside your public/roms folder */}
                <button onClick={() => loadRom("TANK")}>TANK&ensp;</button>
                <button onClick={() => loadRom("TETRIS")}>TETRIS&ensp;</button>
                <button onClick={() => loadRom("INVADERS")}>INVADERS&ensp;</button>
            </div>

            <br />

            <canvas ref={canvasRef} width={640} height={320} />

            <div>
                <p>Controls:</p>
                <ul>
                    <li>1 2 3 4</li>
                    <li>Q W E R</li>
                    <li>A S D F</li>
                    <li>Z X C V</li>
                </ul>
            </div>
        </div>
    );
}
