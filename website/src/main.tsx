import { createRoot } from "react-dom/client";
import "./global.css";
import App from "./routes.tsx";

createRoot(document.getElementById("root")!).render(<App />);
