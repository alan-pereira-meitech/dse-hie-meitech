import { Route, Routes } from "react-router-dom";
import DashboardPage from "./pages/DashboardPage";
import SettingsPage from "./pages/SettingsPage";

function App(): JSX.Element {
  return (
    <Routes>
      <Route path="/" element={<DashboardPage />} />
      <Route path="/settings" element={<SettingsPage />} />
    </Routes>
  );
}

export default App;
