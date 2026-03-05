import { useEffect, useState, useRef } from "react";
import { Terminal, Gamepad2, Grid, Play, Pause, RotateCcw, Code } from "lucide-react";
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

// Inverse map for the visual keypad
const HEX_KEYS = ["1", "2", "3", "C", "4", "5", "6", "D", "7", "8", "9", "E", "A", "0", "B", "F"];

const AVAILABLE_ROMS = [
    "15PUZZLE",
    "BLITZ",
    "CONNECT4",
    "HIDDEN",
    "KALEID",
    "MERLIN",
    "PONG",
    "PUZZLE",
    "TANK",
    "TICTAC",
    "VBRIX",
    "WIPEOFF",
    "BLINKY",
    "BRIX",
    "GUESS",
    "INVADERS",
    "MAZE",
    "MISSILE",
    "PONG2",
    "SYZYGY",
    "TETRIS",
    "UFO",
    "VERS",
];

export default function Home() {
    const [chip8Module, setChip8Module] = useState<Chip8Module | null>(null);
    const [chip8Instance, setChip8Instance] = useState<Chip8 | null>(null);
    const [isRunning, setIsRunning] = useState(false);
    const [activeRom, setActiveRom] = useState<string | null>(null);
    const [speed, setSpeed] = useState(10); // Cycles per frame
    const [logs, setLogs] = useState<string[]>(["[0.000] system.init_start()"]);

    const canvasRef = useRef<HTMLCanvasElement>(null);
    const requestRef = useRef<number>(0);
    const logsEndRef = useRef<HTMLDivElement>(null);
    const logsContainerRef = useRef<HTMLDivElement>(null);
    const isInitialMount = useRef(true);

    const addLog = (msg: string) => {
        setLogs((prev) => [...prev, `[${(performance.now() / 1000).toFixed(3)}] ${msg}`]);
    };

    // Auto-scroll logs (but not on initial mount)
    useEffect(() => {
        if (isInitialMount.current) {
            isInitialMount.current = false;
            return;
        }
        // Scroll within the logs container only, not the page
        if (logsContainerRef.current) {
            logsContainerRef.current.scrollTop = logsContainerRef.current.scrollHeight;
        }
    }, [logs]);

    // Initialize the WASM module
    useEffect(() => {
        let chip8: Chip8;
        addLog("memory.alloc(4096)");

        createModule().then((Module: Chip8Module) => {
            setChip8Module(Module);
            chip8 = new Module.Chip8();
            setChip8Instance(chip8);
            addLog("wasm_module.loaded()");
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

        const handleKeyDown = (e: KeyboardEvent) => {
            const keyIndex = KEY_MAP[e.key.toLowerCase()];
            if (keyIndex !== undefined) {
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
            for (let i = 0; i < speed; i++) {
                chip8Instance.emulate_cycle();
            }

            if (chip8Instance.drawFlag) {
                const gfxArray = chip8Instance.getGfx();

                // Using the new dark color #222222 for background
                ctx.fillStyle = "#222222";
                ctx.fillRect(0, 0, canvas.width, canvas.height);
                // Using the new primary orange #FA8112 for pixels
                ctx.fillStyle = "#FA8112";

                const scale = canvas.width / 64; // Dynamic scale based on canvas size

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

        requestRef.current = requestAnimationFrame(loop);

        return () => {
            window.removeEventListener("keydown", handleKeyDown);
            window.removeEventListener("keyup", handleKeyUp);
            if (requestRef.current) cancelAnimationFrame(requestRef.current);
        };
    }, [chip8Instance, chip8Module, isRunning, speed]);

    // Load ROM
    const loadRom = async (romFilename: string) => {
        if (!chip8Instance || !chip8Module) return;

        try {
            setIsRunning(false);
            setActiveRom(romFilename);
            if (requestRef.current) cancelAnimationFrame(requestRef.current);
            addLog(`rom.fetch("${romFilename}")`);

            const response = await fetch(`/roms/${romFilename}`);
            if (!response.ok) throw new Error(`Could not find /roms/${romFilename}`);

            const buffer = await response.arrayBuffer();
            const uint8View = new Uint8Array(buffer);

            const vec = new chip8Module.VectorUint8();
            for (let i = 0; i < uint8View.length; i++) {
                vec.push_back(uint8View[i]);
            }

            chip8Instance.load_rom_from_buffer(vec);
            vec.delete();

            const canvas = canvasRef.current;
            const ctx = canvas?.getContext("2d");
            if (ctx && canvas) {
                ctx.fillStyle = "#222222";
                ctx.fillRect(0, 0, canvas.width, canvas.height);
            }

            addLog(`emulation.start("${romFilename}")`);
            setIsRunning(true);
        } catch (error) {
            addLog(`ERROR: Failed to load ${romFilename}`);
            console.error("ROM Load Error:", error);
        }
    };

    const handleVisualKey = (hexStr: string, isDown: boolean) => {
        if (!chip8Instance) return;
        const keyIndex = parseInt(hexStr, 16);
        if (!isNaN(keyIndex)) {
            const keys = chip8Instance.getKeys();
            keys[keyIndex] = isDown ? 1 : 0;
        }
    };

    // Custom inline styles for the theme
    const gridStyle = {
        backgroundSize: "20px 20px",
        backgroundImage: "linear-gradient(to right, rgba(250, 129, 18, 0.05) 1px, transparent 1px), linear-gradient(to bottom, rgba(250, 129, 18, 0.05) 1px, transparent 1px)",
    };

    const scanlineStyle = {
        background: "linear-gradient(to bottom, transparent 50%, rgba(0, 0, 0, 0.3) 50%)",
        backgroundSize: "100% 4px",
        pointerEvents: "none" as const,
    };

    if (!chip8Module) {
        return <div className="min-h-screen bg-background-dark flex items-center justify-center text-primary font-mono">Booting Core System...</div>;
    }

    return (
        <div className="relative flex h-dvh w-full flex-col overflow-scroll bg-background-dark font-display text-background-light">
            {/* Top Navigation */}
            <header className="flex items-center justify-between border-b border-primary/20 px-6 py-4 bg-background-dark/80 backdrop-blur-md sticky top-0 z-50">
                <div className="flex items-center gap-4">
                    <div className="text-primary">
                        <Terminal size={32} className="text-primary" />
                    </div>
                    <div>
                        <h2 className="text-primary text-xl font-bold leading-tight tracking-tighter uppercase">NeonCore_v2.0</h2>
                        <p className="text-primary/60 text-xs font-mono">CHIP-8 EMULATION ENVIRONMENT</p>
                    </div>
                </div>
                <div className="flex items-center gap-6">
                    <div className="hidden md:flex items-center gap-4">
                        <div className="flex flex-col items-end">
                            <span className="text-[10px] text-primary/50 uppercase">Emulation Status</span>
                            <div className="flex items-center gap-2 mt-1">
                                <span className={`size-2 rounded-full ${isRunning ? "bg-primary shadow-[0_0_8px_#FA8112]" : "bg-red-500"}`}></span>
                                <span className="text-xs text-primary font-mono">{isRunning ? "RUNNING" : "HALTED"}</span>
                            </div>
                        </div>
                    </div>
                </div>
            </header>

            <main className="flex-1 p-6 grid grid-cols-1 lg:grid-cols-12 gap-6 overflow-y-auto lg:overflow-hidden">
                {/* Left Panel: Sidebar Nav & ROMs */}
                <div className="lg:col-span-3 flex flex-col gap-6 overflow-y-scroll lg:overflow-hidden min-h-72">
                    <div className="border border-primary/20 bg-background-dark p-4 rounded shadow-lg">
                        <div className="flex items-center gap-3 mb-4">
                            <div className={`size-3 rounded-full shadow-[0_0_10px_#FA8112] ${isRunning ? "bg-primary animate-pulse" : "bg-background-surface"}`}></div>
                            <span className="text-sm font-bold uppercase tracking-widest text-primary">System Online</span>
                        </div>
                        <div className="space-y-2 font-mono text-xs">
                            <div className="flex justify-between">
                                <span className="text-primary/50">ACTIVE ROM:</span>
                                <span className="text-primary">{activeRom || "NONE"}</span>
                            </div>
                            <div className="flex justify-between">
                                <span className="text-primary/50">TARGET FPS:</span>
                                <span className="text-primary">60</span>
                            </div>
                        </div>
                    </div>

                    {/* ROM Library */}
                    <div className="flex-1 border border-primary/20 bg-background-dark rounded flex flex-col overflow-hidden shadow-lg">
                        <div className="p-4 border-b border-primary/20 flex justify-between items-center bg-primary/5">
                            <h3 className="text-sm font-bold uppercase tracking-widest text-primary">ROM Library</h3>
                            <span className="text-[10px] bg-primary/20 text-primary px-2 py-0.5 rounded">4096 B</span>
                        </div>
                        <div className="flex-1 overflow-y-auto p-2 space-y-1">
                            {AVAILABLE_ROMS.map((rom) => (
                                <button
                                    key={rom}
                                    onClick={() => loadRom(rom)}
                                    className={`w-full flex items-center gap-3 p-2 rounded font-bold text-sm transition-colors text-left border ${
                                        activeRom === rom ? "bg-primary text-background-dark border-primary" : "hover:bg-primary/10 text-primary/80 border-transparent hover:border-primary/30"
                                    }`}
                                >
                                    <Gamepad2 size={16} className="flex-shrink-0" />
                                    <span>{rom}.CH8</span>
                                </button>
                            ))}
                        </div>
                    </div>
                </div>

                {/* Center Panel: Emulator Display */}
                <div className="lg:col-span-6 flex flex-col gap-6">
                    <div className="relative group">
                        <div className="absolute -inset-1 bg-primary/20 blur opacity-25 group-hover:opacity-40 transition duration-1000"></div>
                        <div className="relative border border-primary/40 bg-background-dark rounded-lg overflow-hidden shadow-2xl aspect-[2/1] flex items-center justify-center" style={gridStyle}>
                            <div className="w-full h-full relative p-4 flex items-center justify-center">
                                <div className="w-full h-full bg-background-dark relative flex items-center justify-center overflow-hidden border border-primary/20">
                                    <div style={scanlineStyle} className="absolute inset-0 z-10 opacity-40"></div>
                                    {!isRunning && !activeRom && (
                                        <div className="text-primary opacity-20 absolute inset-0 flex items-center justify-center select-none pointer-events-none">
                                            <Grid size={120} className="text-primary" />
                                        </div>
                                    )}
                                    <canvas ref={canvasRef} width={640} height={320} className="w-full h-full object-contain relative z-0 opacity-90" />
                                </div>
                            </div>
                        </div>
                    </div>

                    {/* Controls Bar */}
                    <div className="flex justify-between items-center flex-col gap-5 md:flex-row md:gap-0 p-4 border border-primary/20 bg-background-dark rounded shadow-lg">
                        <div className="flex gap-4">
                            <button
                                onClick={() => setIsRunning(true)}
                                disabled={!activeRom || isRunning}
                                className="flex items-center gap-2 px-4 py-2 bg-primary text-background-dark rounded font-bold uppercase text-xs disabled:opacity-50 disabled:cursor-not-allowed hover:bg-primary/80 transition-colors"
                            >
                                <Play size={16} /> Run
                            </button>
                            <button
                                onClick={() => setIsRunning(false)}
                                disabled={!isRunning}
                                className="flex items-center gap-2 px-4 py-2 border border-primary/40 text-primary rounded font-bold uppercase text-xs hover:bg-primary/10 disabled:opacity-50 transition-colors"
                            >
                                <Pause size={16} /> Pause
                            </button>
                            <button
                                onClick={() => activeRom && loadRom(activeRom)}
                                disabled={!activeRom}
                                className="flex items-center gap-2 px-4 py-2 border border-primary/40 text-primary rounded font-bold uppercase text-xs hover:bg-primary/10 disabled:opacity-50 transition-colors"
                            >
                                <RotateCcw size={16} /> Reset
                            </button>
                        </div>
                        <div className="flex items-center gap-4 w-10/12 md:w-fit">
                            <span className="text-xs font-mono text-primary/70">Speed: {speed * 60}Hz</span>
                            <input className="w-full md:w-24" type="range" min="1" max="30" value={speed} onChange={(e) => setSpeed(Number(e.target.value))} />
                        </div>
                    </div>
                </div>

                {/* Right Panel: Keypad & Debug */}
                <div className="lg:col-span-3 flex flex-col gap-6">
                    {/* Hex Keypad */}
                    <div className="border border-primary/20 bg-background-dark rounded flex flex-col h-fit shadow-lg">
                        <div className="p-4 border-b border-primary/20 bg-primary/5">
                            <h3 className="text-sm font-bold uppercase tracking-widest text-primary text-center">Hex Input Map</h3>
                        </div>
                        <div className="p-6 grid grid-cols-4 gap-2">
                            {HEX_KEYS.map((key) => (
                                <button
                                    key={key}
                                    onMouseDown={() => handleVisualKey(key, true)}
                                    onMouseUp={() => handleVisualKey(key, false)}
                                    onMouseLeave={() => handleVisualKey(key, false)}
                                    onTouchStart={() => handleVisualKey(key, true)}
                                    onTouchEnd={() => handleVisualKey(key, false)}
                                    className="aspect-square flex items-center justify-center border border-primary/30 rounded text-primary hover:bg-primary hover:text-background-dark active:bg-primary/80 transition-all font-mono font-bold select-none cursor-pointer"
                                >
                                    {key}
                                </button>
                            ))}
                        </div>
                    </div>

                    {/* Debug Console */}
                    <div className="flex-1 border border-primary/20 bg-background-dark rounded flex flex-col min-h-[200px] shadow-lg">
                        <div className="p-2 border-b border-primary/20 bg-primary/5 flex items-center gap-2">
                            <Code size={12} className="text-primary" />
                            <h3 className="text-[10px] font-bold uppercase tracking-widest text-primary">Kernel Output</h3>
                        </div>
                        <div ref={logsContainerRef} className="p-3 font-mono text-[10px] text-primary/70 space-y-1 overflow-y-auto h-[200px]">
                            {logs.map((log, i) => (
                                <p key={i} className="text-primary/60">
                                    {log}
                                </p>
                            ))}
                            <p className="text-primary animate-pulse mt-2">{">"}</p>
                            <div ref={logsEndRef} />
                        </div>
                    </div>
                </div>
            </main>

            <footer className="mt-auto px-6 py-2 border-t border-primary/10 flex justify-between text-[10px] uppercase tracking-widest text-primary/30 font-mono">
                <div>CHIP-8 Virtual Core v2.0.4-stable</div>
                <div className="flex gap-4">
                    <span>Target FPS: 60</span>
                    <span>WASM: Active</span>
                </div>
            </footer>
        </div>
    );
}
