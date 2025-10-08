import { Link } from "react-router-dom";
import ActionsGrid from "../components/ActionsGrid";
import LogPanel from "../components/LogPanel";
import PageContainer from "../components/PageContainer";

function SettingsPage(): JSX.Element {
  return (
    <PageContainer
      maxWidth="2xl"
      contentClassName="gap-5"
      title="Painel de Ações"
    >
  <div className="flex shrink-0 flex-wrap items-center justify-between gap-4">
        <Link
          to="/"
          className="rounded-2xl border border-slate-500/40 bg-slate-900/70 px-5 py-2.5 text-base text-slate-200 transition-transform hover:-translate-y-0.5 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-slate-200"
        >
          ← Voltar ao dashboard
        </Link>
      </div>

      <div className="grid flex-1 min-h-0 grid-cols-1 gap-8 auto-rows-fr xl:grid-cols-12 xl:items-start">
        <ActionsGrid className="h-full xl:col-span-9" />
        <LogPanel className="h-full xl:col-span-3" />
      </div>
    </PageContainer>
  );
}

export default SettingsPage;
