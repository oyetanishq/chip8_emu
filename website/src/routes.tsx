import { StrictMode } from "react";
import { BrowserRouter, Route, Routes } from "react-router";

import Home from "@/pages/home";
import NotFound from "@/pages/not-found";

export default function App() {
    return (
        <StrictMode>
            <BrowserRouter>
                <Routes>
                    {/* home route */}
                    <Route path="/" element={<Home />} />

                    {/* not found page */}
                    <Route path="*" element={<NotFound />} />
                </Routes>
            </BrowserRouter>
        </StrictMode>
    );
}
