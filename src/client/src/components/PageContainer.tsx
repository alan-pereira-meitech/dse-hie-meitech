import type { PropsWithChildren } from "react";
import { cn } from "../utils/cn";

type PageContainerProps = PropsWithChildren<{
  className?: string;
  contentClassName?: string;
  title?: string;
  subtitle?: string;
  maxWidth?: "lg" | "xl" | "2xl";
}>;

const MAX_WIDTH_MAP: Record<NonNullable<PageContainerProps["maxWidth"]>, string> = {
  lg: "max-w-[1200px]",
  xl: "max-w-[1440px]",
  "2xl": "max-w-[1600px]"
};

function PageContainer({
  children,
  className,
  contentClassName,
  title = "DSE JetBus Meitech",
  subtitle = "Monitore e controle o dispositivo JetBus em tempo real diretamente do navegador.",
  maxWidth = "xl"
}: PageContainerProps): JSX.Element {
  return (
    <div className={cn("flex min-h-screen flex-col overflow-hidden bg-midnight text-slate-100", className)}>
      <header className="shrink-0 px-6 py-5 sm:px-10">
      </header>
      <main
        className={cn(
          "mx-auto flex w-full flex-1 flex-col gap-8 overflow-hidden px-6 pb-16 sm:px-10",
          MAX_WIDTH_MAP[maxWidth],
          contentClassName
        )}
      >
        {children}
      </main>
    </div>
  );
}

export default PageContainer;
