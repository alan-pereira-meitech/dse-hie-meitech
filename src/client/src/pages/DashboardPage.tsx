import { Link } from "react-router-dom";
import ConnectionCard from "../components/ConnectionCard";
import PageContainer from "../components/PageContainer";
import ProcessDataPanel from "../components/ProcessDataPanel";

function DashboardPage(): JSX.Element {
  return (
    <PageContainer
      maxWidth="2xl"
      contentClassName="gap-10"
      title="DSE JetBus Meitech"
      subtitle="Visualize o estado do dispositivo, acompanhe o processo em tempo real e acesse rapidamente ações avançadas."
    >
      <div className="grid grid-cols-1 gap-8 xl:grid-cols-12">
        <ConnectionCard
          size="lg"
          showSettingsLink
          className="col-span-full xl:col-span-4"
        />
        <ProcessDataPanel
          density="spacious"
          showSettingsCta
          className="col-span-full xl:col-span-8"
        />
      </div>

      <section className="panel flex flex-wrap items-center justify-between gap-6">
        <div className="flex-1 min-w-[280px]">
          <h2 className="text-xl font-semibold text-slate-100">Configurações avançadas</h2>
          <p className="mt-2 text-base text-slate-300">
            Precisa calibrar, gravar parâmetros ou executar comandos específicos? Acesse o Painel de Ações para ter
            todas as opções organizadas por categoria, com feedback em tempo real.
          </p>
        </div>
        <Link
          to="/settings"
          className="rounded-2xl bg-gradient-to-r from-sky-400 to-indigo-500 px-6 py-3 text-lg font-semibold text-slate-900 shadow-brand-lg transition-transform hover:-translate-y-0.5 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-sky-300"
        >
          Abrir painel de ações
        </Link>
      </section>
    </PageContainer>
  );
}

export default DashboardPage;
