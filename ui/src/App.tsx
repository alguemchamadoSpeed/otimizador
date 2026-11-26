import type * as React from "react";
import { useEffect, useMemo, useState } from "react";
import {
  Activity, AlertCircle, Archive, ArrowRight, ArrowUpRight, Box, Code2,
  CheckCircle2, ChevronRight, CircleHelp, Command, Cpu, Download, ExternalLink, GitBranch, Globe,
  HardDrive, LayoutDashboard, LoaderCircle, LockKeyhole, MemoryStick, MessageCircle, Monitor, Music,
  Package, PlayCircle, Power, RefreshCw, Rocket, Search, Settings2, ShieldCheck, Sparkles,
  Trash2, Wrench, X, Zap,
} from "lucide-react";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Checkbox } from "@/components/ui/checkbox";
import { Separator } from "@/components/ui/separator";
import { cn, formatBytes } from "@/lib/utils";

type Page = "overview" | "cleaner" | "debloat" | "apps" | "tools";
type Snapshot = {
  windowsName: string; windowsVersion: string; build: number; processor: string;
  memoryTotal: number; memoryAvailable: number; diskFree: number; diskTotal: number;
};
type AppItem = { id: string; name: string; description: string };
type PackageItem = { id: string; name: string; description: string };
type ToastItem = { title: string; description?: string; kind?: "success" | "error" | "info" };
type ConfirmItem = { title: string; description: string; confirmLabel: string; danger?: boolean; onConfirm: () => void };

declare global {
  interface Window {
    chrome?: {
      webview?: {
        postMessage: (message: string) => void;
        addEventListener: (type: "message", listener: (event: MessageEvent) => void) => void;
        removeEventListener: (type: "message", listener: (event: MessageEvent) => void) => void;
      };
    };
  }
}

const demoSnapshot: Snapshot = {
  windowsName: "Windows 11", windowsVersion: "10.0", build: 22631,
  processor: "Processador detectado pelo Windows",
  memoryTotal: 16 * 1024 ** 3, memoryAvailable: 6 * 1024 ** 3,
  diskFree: 350 * 1024 ** 3, diskTotal: 476 * 1024 ** 3,
};

const fallbackApps: AppItem[] = [
  { id: "Microsoft.BingNews", name: "Microsoft Notícias", description: "Notícias e conteúdo editorial" },
  { id: "Microsoft.BingWeather", name: "Clima", description: "Previsão do tempo" },
  { id: "Microsoft.GetHelp", name: "Obter Ajuda", description: "Aplicativo de suporte da Microsoft" },
  { id: "Microsoft.Getstarted", name: "Dicas do Windows", description: "Dicas e introdução ao Windows" },
  { id: "Microsoft.MicrosoftSolitaireCollection", name: "Microsoft Solitaire", description: "Jogos de cartas pré-instalados" },
  { id: "Microsoft.People", name: "Pessoas", description: "Agenda de contatos legada" },
  { id: "Microsoft.Todos", name: "Microsoft To Do", description: "Lista de tarefas" },
  { id: "Microsoft.WindowsFeedbackHub", name: "Hub de Feedback", description: "Envio de comentários para a Microsoft" },
];

const fallbackPackages: PackageItem[] = [
  { id: "Brave.Brave", name: "Brave", description: "Navegador com foco em privacidade" },
  { id: "Discord.Discord", name: "Discord", description: "Comunicação e comunidades" },
  { id: "Microsoft.VisualStudioCode", name: "Visual Studio Code", description: "Editor de código" },
  { id: "Microsoft.VisualStudio.2022.Community", name: "Visual Studio 2022", description: "IDE completa para desenvolvimento" },
  { id: "Git.Git", name: "Git", description: "Controle de versão" },
  { id: "7zip.7zip", name: "7-Zip", description: "Compactador de arquivos" },
  { id: "Microsoft.PowerToys", name: "PowerToys", description: "Utilitários extras para Windows" },
  { id: "VideoLAN.VLC", name: "VLC", description: "Reprodutor multimídia" },
  { id: "Mozilla.Firefox", name: "Firefox", description: "Navegador alternativo" },
  { id: "Spotify.Spotify", name: "Spotify", description: "Aplicativo de música" },
];

const navItems: { id: Page; label: string; icon: typeof LayoutDashboard; group: string }[] = [
  { id: "overview", label: "Visão geral", icon: LayoutDashboard, group: "WORKSPACE" },
  { id: "cleaner", label: "Limpeza", icon: Trash2, group: "WORKSPACE" },
  { id: "debloat", label: "Debloat seletivo", icon: Package, group: "WORKSPACE" },
  { id: "apps", label: "Aplicativos", icon: Download, group: "WORKSPACE" },
  { id: "tools", label: "Ferramentas", icon: Wrench, group: "SYSTEM" },
];

const pageMeta: Record<Page, { title: string; description: string }> = {
  overview: { title: "Visão geral", description: "Um resumo rápido da saúde e dos recursos do seu computador." },
  cleaner: { title: "Limpeza", description: "Libere espaço removendo arquivos temporários e itens dispensáveis." },
  debloat: { title: "Debloat seletivo", description: "Escolha com cuidado os aplicativos opcionais que não fazem parte da sua rotina." },
  apps: { title: "Aplicativos", description: "Instale ferramentas úteis para começar a usar seu Windows." },
  tools: { title: "Ferramentas", description: "Atalhos úteis e utilitários opcionais para completar sua configuração." },
};

function post(message: string) {
  window.chrome?.webview?.postMessage(message);
}

function ProgressBar({ value, tone = "bg-zinc-200" }: { value: number; tone?: string }) {
  return (
    <div className="h-1.5 w-full overflow-hidden rounded-full bg-zinc-800">
      <div className={cn("h-full rounded-full transition-all", tone)} style={{ width: Math.max(0, Math.min(100, value)) + "%" }} />
    </div>
  );
}

function MetricCard({ title, value, detail, icon: Icon, progress, progressTone, footer }: {
  title: string; value: string; detail: string; icon: typeof Cpu; progress?: number; progressTone?: string; footer?: string;
}) {
  return (
    <Card className="min-w-0">
      <CardContent className="p-5">
        <div className="flex items-center justify-between gap-3">
          <span className="text-xs font-medium text-zinc-500">{title}</span>
          <span className="flex size-8 items-center justify-center rounded-lg border border-zinc-800 bg-zinc-900 text-zinc-400"><Icon size={15} /></span>
        </div>
        <div className="mt-4 flex items-baseline gap-2">
          <span className="text-2xl font-semibold tracking-tight text-zinc-100">{value}</span>
          {footer && <span className="text-xs text-zinc-500">{footer}</span>}
        </div>
        <p className="mt-1 min-h-4 text-xs text-zinc-500">{detail}</p>
        {typeof progress === "number" && <div className="mt-4"><ProgressBar value={progress} tone={progressTone} /></div>}
      </CardContent>
    </Card>
  );
}

function SectionHeading({ eyebrow, title, description, action }: {
  eyebrow?: string; title: string; description?: string; action?: React.ReactNode;
}) {
  return (
    <div className="mb-6 flex flex-wrap items-end justify-between gap-4">
      <div className="min-w-0">
        {eyebrow && <div className="mb-2 text-[10px] font-semibold tracking-[0.18em] text-zinc-500">{eyebrow}</div>}
        <h2 className="text-2xl font-semibold tracking-tight text-zinc-100">{title}</h2>
        {description && <p className="mt-1.5 max-w-2xl text-sm text-zinc-500">{description}</p>}
      </div>
      {action}
    </div>
  );
}

function Dialog({ item, busy, onClose }: { item: ConfirmItem | null; busy: boolean; onClose: () => void }) {
  if (!item) return null;
  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 p-4 backdrop-blur-sm" onMouseDown={(event) => { if (event.target === event.currentTarget && !busy) onClose(); }}>
      <div role="dialog" aria-modal="true" aria-labelledby="dialog-title" className="w-full max-w-md rounded-xl border border-zinc-700 bg-[#111115] p-5 shadow-2xl">
        <div className="mb-4 flex size-10 items-center justify-center rounded-lg border border-zinc-800 bg-zinc-900 text-zinc-300">
          <AlertCircle size={18} />
        </div>
        <h3 id="dialog-title" className="text-base font-semibold text-zinc-100">{item.title}</h3>
        <p className="mt-2 text-sm leading-relaxed text-zinc-400">{item.description}</p>
        <div className="mt-6 flex justify-end gap-2">
          <Button variant="outline" disabled={busy} onClick={onClose}>Cancelar</Button>
          <Button variant={item.danger ? "destructive" : "default"} disabled={busy} onClick={item.onConfirm}>
            {busy ? <LoaderCircle className="animate-spin" /> : null}{item.confirmLabel}
          </Button>
        </div>
      </div>
    </div>
  );
}

export default function App() {
  const hasBridge = Boolean(window.chrome?.webview);
  const [page, setPage] = useState<Page>("overview");
  const [snapshot, setSnapshot] = useState<Snapshot>(demoSnapshot);
  const [apps, setApps] = useState<AppItem[]>(fallbackApps);
  const [packages, setPackages] = useState<PackageItem[]>(fallbackPackages);
  const [selectedApps, setSelectedApps] = useState<string[]>([]);
  const [tempBytes, setTempBytes] = useState<number | null>(null);
  const [toast, setToast] = useState<ToastItem | null>(null);
  const [confirm, setConfirm] = useState<ConfirmItem | null>(null);
  const [busy, setBusy] = useState(false);
  const [search, setSearch] = useState("");
  const [lastAction, setLastAction] = useState("");

  const usedMemory = Math.max(0, snapshot.memoryTotal - snapshot.memoryAvailable);
  const memoryPercent = snapshot.memoryTotal > 0 ? usedMemory / snapshot.memoryTotal * 100 : 0;
  const usedDisk = Math.max(0, snapshot.diskTotal - snapshot.diskFree);
  const diskPercent = snapshot.diskTotal > 0 ? usedDisk / snapshot.diskTotal * 100 : 0;

  function notify(item: ToastItem) {
    setToast(item);
    window.setTimeout(() => setToast((current) => current === item ? null : current), 4200);
  }

  function sendAction(message: string) {
    if (!hasBridge) {
      notify({ title: "Prévia da interface", description: "Conecte o aplicativo ao WebView2 para executar ações no Windows.", kind: "info" });
      return;
    }
    if (busy) return;
    setBusy(true);
    setLastAction(message);
    post(message);
  }

  useEffect(() => {
    const webview = window.chrome?.webview;
    if (!webview) return;
    const listener = (event: MessageEvent) => {
      const data = event.data;
      if (!data || typeof data !== "object") return;
      if (data.type === "snapshot") {
        setSnapshot({
          windowsName: data.windowsName ?? "Windows",
          windowsVersion: data.windowsVersion ?? "",
          build: Number(data.build ?? 0),
          processor: data.processor ?? "Não detectado",
          memoryTotal: Number(data.memoryTotal ?? 0),
          memoryAvailable: Number(data.memoryAvailable ?? 0),
          diskFree: Number(data.diskFree ?? 0),
          diskTotal: Number(data.diskTotal ?? 0),
        });
      } else if (data.type === "apps" && Array.isArray(data.items)) {
        setApps(data.items);
      } else if (data.type === "packages" && Array.isArray(data.items)) {
        setPackages(data.items);
      } else if (data.type === "tempInfo") {
        setTempBytes(Number(data.bytes ?? 0));
      } else if (data.type === "actionResult") {
        setBusy(false);
        if (data.refreshSnapshot) post("getSnapshot");
        if (data.tempBytes !== undefined) setTempBytes(Number(data.tempBytes));
        notify({
          title: data.success ? "Concluído" : "Não foi possível concluir",
          description: data.message ?? "A ação foi finalizada.",
          kind: data.success ? "success" : "error",
        });
      }
    };
    webview.addEventListener("message", listener);
    post("getSnapshot");
    post("getApps");
    post("getPackages");
    return () => webview.removeEventListener("message", listener);
  }, [hasBridge]);

  useEffect(() => {
    if (page === "cleaner" && tempBytes === null && hasBridge) post("getTempInfo");
  }, [page, tempBytes, hasBridge]);

  const filteredPackages = useMemo(() => packages.filter((item) => {
    const query = search.trim().toLowerCase();
    return !query || item.name.toLowerCase().includes(query) || item.description.toLowerCase().includes(query);
  }), [packages, search]);

  const filteredApps = useMemo(() => apps.filter((item) => {
    const query = search.trim().toLowerCase();
    return !query || item.name.toLowerCase().includes(query) || item.description.toLowerCase().includes(query);
  }), [apps, search]);

  function requestCleanup() {
    setConfirm({
      title: "Limpar arquivos temporários?",
      description: "O Otimizador removerá somente arquivos da pasta temporária do seu usuário. Arquivos em uso, protegidos ou que sejam links serão ignorados.",
      confirmLabel: "Limpar arquivos",
      onConfirm: () => { setConfirm(null); sendAction("cleanTemp"); },
    });
  }

  function requestRecycleBin() {
    setConfirm({
      title: "Esvaziar a Lixeira?",
      description: "Esta ação é permanente. Confira se não há arquivos que você deseja recuperar antes de continuar.",
      confirmLabel: "Esvaziar Lixeira",
      danger: true,
      onConfirm: () => { setConfirm(null); sendAction("emptyRecycleBin"); },
    });
  }

  function requestDebloat() {
    if (!selectedApps.length) {
      notify({ title: "Nenhum aplicativo selecionado", description: "Marque os aplicativos opcionais que realmente não utiliza.", kind: "info" });
      return;
    }
    const names = apps.filter((app) => selectedApps.includes(app.id)).map((app) => app.name).join(", ");
    setConfirm({
      title: "Remover aplicativos selecionados?",
      description: "Os pacotes serão removidos apenas do seu usuário atual. Alguns podem ser reinstalados pela Microsoft Store. Selecionados: " + names,
      confirmLabel: "Remover selecionados",
      danger: true,
      onConfirm: () => {
        setConfirm(null);
        sendAction("removeApps:" + selectedApps.join("|"));
        setSelectedApps([]);
      },
    });
  }

  const meta = pageMeta[page];

  return (
    <div className="app-shell dark">
      <aside className="sidebar">
        <div className="flex items-center gap-3 px-2 pt-1">
          <div className="flex size-9 shrink-0 items-center justify-center rounded-xl bg-zinc-100 text-zinc-950 shadow-sm"><Command size={19} strokeWidth={2.5} /></div>
          <div className="sidebar-wordmark min-w-0">
            <div className="text-sm font-semibold tracking-wide text-zinc-100">OTIMIZADOR</div>
            <div className="mt-0.5 text-[10px] tracking-[0.14em] text-zinc-500">WINDOWS 10 / 11</div>
          </div>
        </div>

        <div className="mt-3">
          <div className="nav-group-label mb-2 px-3 text-[10px] font-semibold tracking-[0.16em] text-zinc-600">WORKSPACE</div>
          <nav className="space-y-1">
            {navItems.filter((item) => item.group === "WORKSPACE").map((item) => {
              const Icon = item.icon;
              return (
                <button key={item.id} onClick={() => { setPage(item.id); setSearch(""); }} className={cn(
                  "nav-item flex h-10 w-full items-center gap-3 rounded-md px-3 text-left text-[13px] font-medium transition-colors",
                  page === item.id ? "bg-zinc-800 text-zinc-100 shadow-sm" : "text-zinc-500 hover:bg-zinc-900 hover:text-zinc-200",
                )}>
                  <Icon size={16} /><span className="nav-text">{item.label}</span>
                  {page === item.id && <span className="ml-auto size-1.5 rounded-full bg-zinc-200 nav-label" />}
                </button>
              );
            })}
          </nav>
        </div>

        <div>
          <div className="nav-group-label mb-2 px-3 text-[10px] font-semibold tracking-[0.16em] text-zinc-600">SISTEMA</div>
          <button onClick={() => setPage("tools")} className={cn("nav-item flex h-10 w-full items-center gap-3 rounded-md px-3 text-left text-[13px] font-medium transition-colors", page === "tools" ? "bg-zinc-800 text-zinc-100" : "text-zinc-500 hover:bg-zinc-900 hover:text-zinc-200")}>
            <Wrench size={16} /><span className="nav-text">Ferramentas</span>
          </button>
        </div>

        <div className="mt-auto sidebar-footer">
          <Separator className="mb-4" />
          <div className="rounded-lg border border-zinc-800 bg-zinc-900/50 p-3">
            <div className="flex items-center gap-2 text-xs font-medium text-emerald-300"><ShieldCheck size={14} /> Seguro por padrão</div>
            <p className="mt-2 text-[11px] leading-relaxed text-zinc-500">O Otimizador não desativa Defender, firewall ou Windows Update.</p>
          </div>
          <div className="mt-4 flex items-center justify-between px-1 text-[10px] text-zinc-600">
            <span>Versão 1.1.0</span><span>C++ / React</span>
          </div>
        </div>
      </aside>

      <main className="main-scroll">
        <header className="sticky top-0 z-20 flex h-[68px] items-center justify-between border-b border-zinc-800/80 bg-[#09090b]/90 px-7 backdrop-blur-xl">
          <div className="flex min-w-0 items-center gap-3">
            <div className="hidden text-xs text-zinc-600 sm:block">Workspace</div>
            <ChevronRight size={13} className="hidden text-zinc-700 sm:block" />
            <div className="truncate text-sm font-medium text-zinc-200">{meta.title}</div>
          </div>
          <div className="flex items-center gap-2">
            <Badge variant={hasBridge ? "success" : "secondary"} className="hidden sm:inline-flex">
              <span className={cn("size-1.5 rounded-full", hasBridge ? "bg-emerald-400" : "bg-zinc-500")} />
              {hasBridge ? "Windows conectado" : "Modo prévia"}
            </Badge>
            <Button variant="outline" size="icon" title="Atualizar dados" disabled={busy} onClick={() => post("getSnapshot")}><RefreshCw size={15} className={busy ? "animate-spin" : ""} /></Button>
          </div>
        </header>

        <div className="mx-auto max-w-[1500px] px-7 pb-10 pt-8">
          <div className="mb-7 flex flex-wrap items-end justify-between gap-4">
            <div>
              <div className="mb-2 flex items-center gap-2 text-xs text-zinc-500"><span className="size-1.5 rounded-full bg-emerald-400" /> Seu PC em um só lugar</div>
              <h1 className="text-[27px] font-semibold tracking-tight text-zinc-50">{meta.title}</h1>
              <p className="mt-1.5 text-sm text-zinc-500">{meta.description}</p>
            </div>
            {page === "apps" && <div className="relative w-full sm:w-64"><Search size={15} className="absolute left-3 top-1/2 -translate-y-1/2 text-zinc-500" /><input value={search} onChange={(e) => setSearch(e.target.value)} placeholder="Buscar aplicativo..." className="h-9 w-full rounded-md border border-zinc-800 bg-zinc-950 pl-9 pr-3 text-sm text-zinc-200 placeholder:text-zinc-600 outline-none transition focus:border-zinc-600" /></div>}
            {page === "debloat" && <div className="relative w-full sm:w-64"><Search size={15} className="absolute left-3 top-1/2 -translate-y-1/2 text-zinc-500" /><input value={search} onChange={(e) => setSearch(e.target.value)} placeholder="Buscar aplicativo..." className="h-9 w-full rounded-md border border-zinc-800 bg-zinc-950 pl-9 pr-3 text-sm text-zinc-200 placeholder:text-zinc-600 outline-none transition focus:border-zinc-600" /></div>}
          </div>

          {page === "overview" && (
            <div className="space-y-6">
              <Card className="overflow-hidden border-zinc-800 bg-gradient-to-br from-[#17171d] via-[#111115] to-[#0d0d11]">
                <CardContent className="flex flex-wrap items-center justify-between gap-6 p-6 sm:p-7">
                  <div className="max-w-xl">
                    <Badge variant="success" className="mb-3"><CheckCircle2 size={11} /> Sistema pronto</Badge>
                    <h2 className="text-xl font-semibold tracking-tight text-zinc-50">Tudo em ordem por aqui.</h2>
                    <p className="mt-2 text-sm leading-relaxed text-zinc-400">Revise os aplicativos instalados, remova arquivos temporários e prepare seu Windows para o dia a dia.</p>
                    <div className="mt-5 flex flex-wrap gap-2">
                      <Button onClick={() => setPage("cleaner")}><Sparkles size={15} /> Começar limpeza <ArrowRight size={14} /></Button>
                      <Button variant="outline" onClick={() => setPage("apps")}>Configurar aplicativos</Button>
                    </div>
                  </div>
                  <div className="mr-3 hidden size-28 items-center justify-center rounded-full border border-zinc-700/80 bg-zinc-900/80 text-zinc-300 sm:flex">
                    <div className="flex size-20 items-center justify-center rounded-full border border-zinc-700 bg-[#111115]"><Rocket size={32} strokeWidth={1.3} /></div>
                  </div>
                </CardContent>
              </Card>

              <div>
                <div className="mb-3 flex items-center justify-between">
                  <h3 className="text-sm font-semibold text-zinc-200">Recursos do sistema</h3>
                  <span className="text-xs text-zinc-600">Atualizado agora</span>
                </div>
                <div className="grid gap-3 md:grid-cols-3">
                  <MetricCard title="Memória RAM" value={formatBytes(snapshot.memoryTotal)} detail={formatBytes(snapshot.memoryAvailable) + " disponíveis"} icon={MemoryStick} progress={memoryPercent} progressTone="bg-sky-400" footer={Math.round(memoryPercent) + "% em uso"} />
                  <MetricCard title="Armazenamento" value={formatBytes(snapshot.diskFree)} detail={"livres de " + formatBytes(snapshot.diskTotal)} icon={HardDrive} progress={diskPercent} progressTone="bg-violet-400" footer={Math.round(diskPercent) + "% usado"} />
                  <MetricCard title="Sistema operacional" value={snapshot.windowsName} detail={snapshot.windowsVersion + (snapshot.build ? " • Build " + snapshot.build : "")} icon={Monitor} footer="64-bit / Windows" />
                </div>
              </div>

              <div className="grid gap-4 lg:grid-cols-[1.45fr_1fr]">
                <Card>
                  <CardHeader className="pb-3">
                    <div className="flex items-center justify-between">
                      <CardTitle>Primeiros passos</CardTitle>
                      <Badge variant="secondary">Recomendado</Badge>
                    </div>
                    <CardDescription>Uma configuração inicial simples e segura.</CardDescription>
                  </CardHeader>
                  <CardContent className="space-y-2">
                    <button onClick={() => setPage("cleaner")} className="group flex w-full items-center gap-3 rounded-lg border border-zinc-800 bg-zinc-950/60 p-3 text-left transition hover:border-zinc-700 hover:bg-zinc-900/70">
                      <div className="flex size-9 items-center justify-center rounded-lg bg-zinc-900 text-zinc-300"><Trash2 size={17} /></div>
                      <div className="min-w-0 flex-1"><div className="text-sm font-medium text-zinc-200">Limpar temporários</div><div className="mt-0.5 text-xs text-zinc-500">Recupere espaço sem mexer em arquivos pessoais.</div></div>
                      <ArrowUpRight size={16} className="text-zinc-600 transition group-hover:text-zinc-300" />
                    </button>
                    <button onClick={() => setPage("debloat")} className="group flex w-full items-center gap-3 rounded-lg border border-zinc-800 bg-zinc-950/60 p-3 text-left transition hover:border-zinc-700 hover:bg-zinc-900/70">
                      <div className="flex size-9 items-center justify-center rounded-lg bg-zinc-900 text-zinc-300"><Package size={17} /></div>
                      <div className="min-w-0 flex-1"><div className="text-sm font-medium text-zinc-200">Revisar aplicativos opcionais</div><div className="mt-0.5 text-xs text-zinc-500">Remova somente o que você não utiliza.</div></div>
                      <ArrowUpRight size={16} className="text-zinc-600 transition group-hover:text-zinc-300" />
                    </button>
                    <button onClick={() => setPage("apps")} className="group flex w-full items-center gap-3 rounded-lg border border-zinc-800 bg-zinc-950/60 p-3 text-left transition hover:border-zinc-700 hover:bg-zinc-900/70">
                      <div className="flex size-9 items-center justify-center rounded-lg bg-zinc-900 text-zinc-300"><Download size={17} /></div>
                      <div className="min-w-0 flex-1"><div className="text-sm font-medium text-zinc-200">Instalar ferramentas úteis</div><div className="mt-0.5 text-xs text-zinc-500">Navegador, editor, utilitários e mais.</div></div>
                      <ArrowUpRight size={16} className="text-zinc-600 transition group-hover:text-zinc-300" />
                    </button>
                  </CardContent>
                </Card>

                <Card>
                  <CardHeader className="pb-3">
                    <CardTitle>Informações do dispositivo</CardTitle>
                    <CardDescription>Detalhes detectados no Windows.</CardDescription>
                  </CardHeader>
                  <CardContent className="space-y-4">
                    <div className="flex gap-3"><Cpu size={16} className="mt-0.5 text-zinc-500" /><div className="min-w-0"><div className="text-xs text-zinc-500">Processador</div><div className="mt-1 break-words text-sm font-medium text-zinc-200">{snapshot.processor}</div></div></div>
                    <Separator />
                    <div className="flex gap-3"><Monitor size={16} className="mt-0.5 text-zinc-500" /><div><div className="text-xs text-zinc-500">Windows</div><div className="mt-1 text-sm font-medium text-zinc-200">{snapshot.windowsName} {snapshot.windowsVersion}</div></div></div>
                    <Separator />
                    <div className="flex gap-3"><ShieldCheck size={16} className="mt-0.5 text-emerald-400" /><div><div className="text-xs text-zinc-500">Segurança</div><div className="mt-1 text-sm font-medium text-zinc-200">Proteções essenciais preservadas</div></div></div>
                  </CardContent>
                </Card>
              </div>
            </div>
          )}

          {page === "cleaner" && (
            <div className="grid gap-4 xl:grid-cols-[1.35fr_1fr]">
              <Card>
                <CardHeader>
                  <div className="flex items-center justify-between"><CardTitle>Arquivos temporários</CardTitle><Badge variant="success">Baixo risco</Badge></div>
                  <CardDescription>Conteúdo da pasta temporária do seu perfil do Windows.</CardDescription>
                </CardHeader>
                <CardContent>
                  <div className="rounded-lg border border-zinc-800 bg-zinc-950/70 p-5">
                    <div className="flex items-center gap-2 text-xs text-zinc-500"><HardDrive size={14} /> ESPAÇO QUE PODE SER LIBERADO</div>
                    <div className="mt-3 text-3xl font-semibold tracking-tight text-zinc-100">{tempBytes === null ? "—" : formatBytes(tempBytes)}</div>
                    <p className="mt-2 text-xs text-zinc-500">Local: %LOCALAPPDATA%\Temp</p>
                    <div className="mt-5 flex flex-wrap gap-2">
                      <Button disabled={busy} onClick={requestCleanup}><Trash2 size={15} /> Limpar temporários</Button>
                      <Button variant="outline" disabled={busy} onClick={() => post("getTempInfo")}><RefreshCw size={14} /> Verificar novamente</Button>
                    </div>
                  </div>
                  <div className="mt-4 flex gap-3 rounded-lg border border-zinc-800 p-4">
                    <ShieldCheck size={17} className="mt-0.5 shrink-0 text-emerald-400" />
                    <div><div className="text-sm font-medium text-zinc-200">Limpeza com limites</div><p className="mt-1 text-xs leading-relaxed text-zinc-500">Arquivos em uso ou protegidos são ignorados. Links simbólicos e junctions não são percorridos. Seus documentos e downloads não são tocados.</p></div>
                  </div>
                </CardContent>
              </Card>
              <Card>
                <CardHeader><CardTitle>Lixeira do Windows</CardTitle><CardDescription>Remova os itens que já foram enviados à Lixeira.</CardDescription></CardHeader>
                <CardContent>
                  <div className="flex size-11 items-center justify-center rounded-xl border border-zinc-800 bg-zinc-900 text-zinc-300"><Trash2 size={20} /></div>
                  <h3 className="mt-4 text-base font-semibold text-zinc-100">Esvaziar a Lixeira</h3>
                  <p className="mt-2 text-sm leading-relaxed text-zinc-500">Esta operação é permanente e não pode ser desfeita pelo Otimizador. Confira os arquivos antes de continuar.</p>
                  <Button className="mt-5" variant="outline" disabled={busy} onClick={requestRecycleBin}>Esvaziar Lixeira</Button>
                </CardContent>
              </Card>
              <Card className="xl:col-span-2">
                <CardContent className="flex flex-wrap items-center justify-between gap-3 p-5">
                  <div className="flex items-center gap-3"><div className="flex size-9 items-center justify-center rounded-lg bg-zinc-900 text-zinc-400"><CircleHelp size={17} /></div><div><div className="text-sm font-medium text-zinc-200">Por que não limpar tudo?</div><p className="mt-1 text-xs text-zinc-500">Caches de aplicativos podem ser necessários para inicialização mais rápida. O Otimizador evita apagar pastas de programas indiscriminadamente.</p></div></div>
                  <Badge variant="secondary">Seguro por padrão</Badge>
                </CardContent>
              </Card>
            </div>
          )}

          {page === "debloat" && (
            <div className="space-y-4">
              <Card>
                <CardHeader className="flex-row items-center justify-between">
                  <div><CardTitle>Aplicativos opcionais</CardTitle><CardDescription className="mt-1">Selecione apenas os que você não usa. A remoção é limitada à sua conta.</CardDescription></div>
                  <Badge variant="secondary">{selectedApps.length} selecionado(s)</Badge>
                </CardHeader>
                <CardContent>
                  <div className="mb-3 flex items-center justify-between text-[11px] font-medium uppercase tracking-wider text-zinc-600"><span>Aplicativo</span><span>Selecionar</span></div>
                  <div className="divide-y divide-zinc-800/80 rounded-lg border border-zinc-800">
                    {filteredApps.map((app) => (
                      <label key={app.id} className="flex cursor-pointer items-center gap-3 px-4 py-3 transition hover:bg-zinc-900/60">
                        <div className="flex size-9 shrink-0 items-center justify-center rounded-lg border border-zinc-800 bg-zinc-950 text-zinc-500"><Package size={16} /></div>
                        <div className="min-w-0 flex-1"><div className="text-sm font-medium text-zinc-200">{app.name}</div><div className="mt-0.5 text-xs text-zinc-500">{app.description}</div></div>
                        <Checkbox checked={selectedApps.includes(app.id)} onCheckedChange={(checked) => setSelectedApps((current) => checked ? [...current, app.id] : current.filter((id) => id !== app.id))} />
                      </label>
                    ))}
                    {!filteredApps.length && <div className="p-8 text-center text-sm text-zinc-500">Nenhum aplicativo encontrado.</div>}
                  </div>
                  <div className="mt-4 flex flex-wrap items-center justify-between gap-3">
                    <p className="max-w-xl text-xs leading-relaxed text-zinc-500">Não são listados Microsoft Store, Edge, Defender, WebView2, drivers ou componentes do Windows Update.</p>
                    <Button disabled={busy || selectedApps.length === 0} onClick={requestDebloat}><Trash2 size={15} /> Remover selecionados</Button>
                  </div>
                </CardContent>
              </Card>
              <div className="flex gap-3 rounded-lg border border-amber-900/50 bg-amber-950/20 p-4 text-amber-200">
                <AlertCircle size={17} className="mt-0.5 shrink-0" /><p className="text-xs leading-relaxed text-amber-200/75">Alguns aplicativos pré-instalados podem ser úteis. A remoção pode exigir reinstalação pela Microsoft Store. Revise a seleção antes de confirmar.</p>
              </div>
            </div>
          )}

          {page === "apps" && (
            <div className="space-y-5">
              <div className="flex flex-wrap items-center justify-between gap-3">
                <div className="flex items-center gap-2 text-xs text-zinc-500"><Badge variant={hasBridge ? "success" : "secondary"}>{hasBridge ? "winget verificado no Windows" : "Catálogo de prévia"}</Badge><span>Instaladores oficiais via Windows Package Manager.</span></div>
                <Button variant="outline" size="sm" onClick={() => post("open:winget")}><ExternalLink size={13} /> Sobre o winget</Button>
              </div>
              <div className="grid gap-3 md:grid-cols-2 xl:grid-cols-3">
                {filteredPackages.map((item, index) => {
                  const icons = [Command, MessageCircle, Code2, Monitor, GitBranch, Archive, Settings2, PlayCircle, Globe, Music];
                  const Icon = icons[index % icons.length];
                  return (
                    <Card key={item.id} className="group transition hover:border-zinc-700 hover:bg-[#131318]">
                      <CardContent className="p-4">
                        <div className="flex items-start justify-between">
                          <div className="flex size-10 items-center justify-center rounded-xl border border-zinc-800 bg-zinc-900 text-zinc-300"><Icon size={18} /></div>
                          <Badge variant="outline">Winget</Badge>
                        </div>
                        <h3 className="mt-4 text-sm font-semibold text-zinc-100">{item.name}</h3>
                        <p className="mt-1 min-h-8 text-xs leading-relaxed text-zinc-500">{item.description}</p>
                        <Separator className="my-3" />
                        <div className="flex items-center justify-between gap-2">
                          <span className="max-w-[65%] truncate text-[10px] text-zinc-600">{item.id}</span>
                          <Button size="sm" variant="secondary" disabled={busy} onClick={() => sendAction("install:" + item.id)}><Download size={13} /> Instalar</Button>
                        </div>
                      </CardContent>
                    </Card>
                  );
                })}
                {!filteredPackages.length && <div className="col-span-full rounded-lg border border-zinc-800 p-8 text-center text-sm text-zinc-500">Nenhum aplicativo encontrado.</div>}
              </div>
            </div>
          )}

          {page === "tools" && (
            <div className="grid gap-4 md:grid-cols-2">
              <ToolCard icon={Power} title="Aplicativos de inicialização" description="Revise quais programas iniciam junto com o Windows." action="Abrir configurações" onClick={() => post("open:startup")} />
              <ToolCard icon={Box} title="Aplicativos instalados" description="Veja, altere ou desinstale programas instalados no computador." action="Gerenciar aplicativos" onClick={() => post("open:apps")} />
              <ToolCard icon={ShieldCheck} title="Segurança do Windows" description="Abra o painel oficial para conferir proteção e status de segurança." action="Abrir segurança" onClick={() => post("open:security")} />
              <ToolCard icon={Activity} title="Windows Update" description="Confira atualizações disponíveis e reinicializações pendentes." action="Ver atualizações" onClick={() => post("open:update")} />
              <ToolCard icon={Sparkles} title="SpotifyLite" description="Projeto de terceiros. Confira o release, o publicador e o hash antes de executar." action="Ver release no GitHub" onClick={() => post("open:spotifyLite")} external />
              <ToolCard icon={Zap} title="DiscordDebloat v2.1" description="Ferramenta de terceiros para personalização do Discord. Leia as notas do release." action="Ver release no GitHub" onClick={() => post("open:discordDebloat")} external />
              <div className="md:col-span-2 rounded-lg border border-zinc-800 bg-zinc-950/60 p-4">
                <div className="flex items-center gap-2 text-sm font-medium text-zinc-200"><LockKeyhole size={15} className="text-emerald-400" /> Princípio de segurança</div>
                <p className="mt-1 text-xs leading-relaxed text-zinc-500">O Otimizador não altera serviços essenciais, timers do sistema, políticas de segurança, Defender, firewall ou Windows Update para prometer ganhos artificiais de FPS.</p>
              </div>
            </div>
          )}

          <div className="mt-8 flex flex-wrap items-center justify-between gap-2 border-t border-zinc-800/70 pt-4 text-[10px] text-zinc-600">
            <span>Otimizador • Ferramentas de manutenção para Windows 10 e 11</span>
            <span className="flex items-center gap-1.5"><ShieldCheck size={12} /> Ações com confirmação</span>
          </div>
        </div>
      </main>

      {busy && <div className="fixed bottom-5 right-5 z-40 flex items-center gap-2 rounded-lg border border-zinc-700 bg-zinc-900 px-3 py-2 text-xs text-zinc-300 shadow-xl"><LoaderCircle size={14} className="animate-spin" /> Processando ação no Windows…</div>}
      {toast && <div className={cn("fixed bottom-5 right-5 z-50 max-w-sm rounded-lg border px-4 py-3 shadow-2xl", toast.kind === "error" ? "border-red-900 bg-[#1b1012]" : "border-zinc-700 bg-[#151519]")}>
        <div className="flex items-start gap-2.5">
          {toast.kind === "success" ? <CheckCircle2 size={16} className="mt-0.5 text-emerald-400" /> : toast.kind === "error" ? <AlertCircle size={16} className="mt-0.5 text-red-400" /> : <CircleHelp size={16} className="mt-0.5 text-zinc-400" />}
          <div className="min-w-0 flex-1"><div className="text-sm font-medium text-zinc-100">{toast.title}</div>{toast.description && <p className="mt-1 text-xs leading-relaxed text-zinc-400">{toast.description}</p>}</div>
          <button onClick={() => setToast(null)} className="text-zinc-500 hover:text-zinc-200"><X size={14} /></button>
        </div>
      </div>}
      <Dialog item={confirm} busy={busy} onClose={() => setConfirm(null)} />
    </div>
  );
}

function ToolCard({ icon: Icon, title, description, action, onClick, external = false }: {
  icon: typeof Wrench; title: string; description: string; action: string; onClick: () => void; external?: boolean;
}) {
  return (
    <Card className="flex min-h-[190px] flex-col">
      <CardContent className="flex flex-1 flex-col p-5">
        <div className="flex size-10 items-center justify-center rounded-xl border border-zinc-800 bg-zinc-900 text-zinc-300"><Icon size={18} /></div>
        <h3 className="mt-4 text-sm font-semibold text-zinc-100">{title}</h3>
        <p className="mt-1 flex-1 text-xs leading-relaxed text-zinc-500">{description}</p>
        <Button className="mt-4 w-fit" variant="outline" size="sm" onClick={onClick}>{action}{external ? <ExternalLink size={13} /> : <ArrowRight size={13} />}</Button>
      </CardContent>
    </Card>
  );
}

