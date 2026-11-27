#requires -Version 5.1
# Otimizador para Windows 10/11 - PowerShell WinForms.
# Acoes destrutivas pedem confirmacao e usam caminhos/listas fixas.

$ErrorActionPreference = 'Stop'

try {
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    [System.System.Windows.Forms.Application]::EnableVisualStyles()
    [System.System.Windows.Forms.Application]::SetCompatibleTextRenderingDefault($false)
}
catch {
    $msg = 'Nao foi possivel carregar os componentes graficos do Windows.' + [Environment]::NewLine + $_.Exception.Message
    [System.System.Windows.Forms.MessageBox]::Show($msg, 'Otimizador - erro', 'OK', 'Error') | Out-Null
    exit 1
}

$script:T = @{
    Bg = [System.Drawing.Color]::FromArgb(9, 13, 21)
    Sidebar = [System.Drawing.Color]::FromArgb(12, 18, 29)
    Card = [System.Drawing.Color]::FromArgb(18, 27, 41)
    CardAlt = [System.Drawing.Color]::FromArgb(21, 34, 53)
    Border = [System.Drawing.Color]::FromArgb(39, 55, 76)
    Text = [System.Drawing.Color]::FromArgb(240, 244, 252)
    Muted = [System.Drawing.Color]::FromArgb(145, 160, 183)
    Accent = [System.Drawing.Color]::FromArgb(112, 132, 255)
    AccentDark = [System.Drawing.Color]::FromArgb(35, 47, 83)
    Green = [System.Drawing.Color]::FromArgb(78, 205, 158)
    Orange = [System.Drawing.Color]::FromArgb(245, 177, 93)
}
$script:F = @{
    Title = [System.Drawing.Font]::new('Segoe UI', 23, [System.Drawing.FontStyle]::Bold)
    Heading = [System.Drawing.Font]::new('Segoe UI', 12, [System.Drawing.FontStyle]::Bold)
    Body = [System.Drawing.Font]::new('Segoe UI', 9, [System.Drawing.FontStyle]::Regular)
    Small = [System.Drawing.Font]::new('Segoe UI', 8, [System.Drawing.FontStyle]::Regular)
    Button = [System.Drawing.Font]::new('Segoe UI', 9, [System.Drawing.FontStyle]::Semibold)
}
$script:Form = $null
$script:Content = $null
$script:Sidebar = $null
$script:Status = $null
$script:Nav = @{}
$script:PageName = 'Home'
$script:DebloatItems = @()
$script:TempSizeLabel = $null
$script:DebloatCount = $null

$script:OptionalApps = @(
    [pscustomobject]@{ Id='Microsoft.BingNews'; Name='Microsoft Noticias'; Description='Noticias e conteudo editorial' },
    [pscustomobject]@{ Id='Microsoft.BingWeather'; Name='Clima'; Description='Previsao do tempo' },
    [pscustomobject]@{ Id='Microsoft.GetHelp'; Name='Obter Ajuda'; Description='Aplicativo de suporte da Microsoft' },
    [pscustomobject]@{ Id='Microsoft.Getstarted'; Name='Dicas do Windows'; Description='Dicas e introducao ao Windows' },
    [pscustomobject]@{ Id='Microsoft.MicrosoftSolitaireCollection'; Name='Microsoft Solitaire'; Description='Jogos de cartas pre-instalados' },
    [pscustomobject]@{ Id='Microsoft.People'; Name='Pessoas'; Description='Agenda de contatos legada' },
    [pscustomobject]@{ Id='Microsoft.Todos'; Name='Microsoft To Do'; Description='Lista de tarefas; remova somente se nao usar' },
    [pscustomobject]@{ Id='Microsoft.WindowsFeedbackHub'; Name='Hub de Feedback'; Description='Envio de comentarios para a Microsoft' }
)
$script:Packages = @(
    [pscustomobject]@{ Name='Brave Browser'; Id='Brave.Brave'; Description='Navegador com foco em privacidade' },
    [pscustomobject]@{ Name='Firefox'; Id='Mozilla.Firefox'; Description='Navegador alternativo' },
    [pscustomobject]@{ Name='Discord'; Id='Discord.Discord'; Description='Comunicacao e comunidades' },
    [pscustomobject]@{ Name='Visual Studio Code'; Id='Microsoft.VisualStudioCode'; Description='Editor de codigo leve' },
    [pscustomobject]@{ Name='Git'; Id='Git.Git'; Description='Controle de versao' },
    [pscustomobject]@{ Name='7-Zip'; Id='7zip.7zip'; Description='Compactador de arquivos' },
    [pscustomobject]@{ Name='PowerToys'; Id='Microsoft.PowerToys'; Description='Utilitarios extras para Windows' },
    [pscustomobject]@{ Name='VLC'; Id='VideoLAN.VLC'; Description='Reprodutor multimidia' },
    [pscustomobject]@{ Name='Spotify'; Id='Spotify.Spotify'; Description='Aplicativo de musica' },
    [pscustomobject]@{ Name='Visual Studio 2022 Community'; Id='Microsoft.VisualStudio.2022.Community'; Description='IDE completa; download grande' }
)

function Set-Status([string]$Message) {
    if ($script:Status) {
        $script:Status.Text = '  ' + $Message
        $script:Status.Refresh()
    }
}
function Show-Info([string]$Message, [string]$Title='Otimizador') {
    [System.System.Windows.Forms.MessageBox]::Show($script:Form, $Message, $Title, [System.System.Windows.Forms.MessageBoxButtons]::OK, [System.System.Windows.Forms.MessageBoxIcon]::Information) | Out-Null
}
function Show-Warning([string]$Message, [string]$Title='Otimizador') {
    [System.System.Windows.Forms.MessageBox]::Show($script:Form, $Message, $Title, [System.System.Windows.Forms.MessageBoxButtons]::OK, [System.System.Windows.Forms.MessageBoxIcon]::Warning) | Out-Null
}
function Confirm-Action([string]$Message, [string]$Title='Confirmar acao') {
    $answer = [System.System.Windows.Forms.MessageBox]::Show($script:Form, $Message, $Title, [System.System.Windows.Forms.MessageBoxButtons]::YesNo, [System.System.Windows.Forms.MessageBoxIcon]::Warning, [System.System.Windows.Forms.MessageBoxDefaultButton]::Button2)
    return ($answer -eq [System.System.Windows.Forms.DialogResult]::Yes)
}
function Format-Bytes([long]$Bytes) {
    $n = [double]$Bytes
    $units = @('B','KB','MB','GB','TB')
    $i = 0
    while ($n -ge 1024 -and $i -lt 4) { $n /= 1024; $i++ }
    return ('{0:N1} {1}' -f $n, $units[$i])
}

function Get-SystemSnapshot {
    $name='Windows'; $version='Versao indisponivel'; $cpu='Nao detectado'
    $memory='Nao detectada'; $memoryDetail=''; $free='Indisponivel'; $diskDetail=''
    try {
        $os=Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion'
        if ($os.ProductName) { $name=$os.ProductName -replace '^Microsoft ',' }
        $v=if ($os.DisplayVersion) {$os.DisplayVersion} else {$os.ReleaseId}
        $version="$v | Build $($os.CurrentBuild)"
    } catch {}
    try {
        $p=Get-CimInstance Win32_Processor -ErrorAction Stop | Select-Object -First 1
        if ($p.Name) {$cpu=($p.Name -replace '\s+',' ').Trim()}
    } catch {
        try {$p=Get-ItemProperty 'HKLM:\HARDWARE\DESCRIPTION\System\CentralProcessor\0'; if($p.ProcessorNameString){$cpu=$p.ProcessorNameString.Trim()}} catch {}
    }
    try {
        $cs=Get-CimInstance Win32_ComputerSystem -ErrorAction Stop
        $memory=('{0:N1} GB' -f ($cs.TotalPhysicalMemory/1GB))
        try {$osInfo=Get-CimInstance Win32_OperatingSystem -ErrorAction Stop; $memoryDetail=('{0:N1} GB disponiveis' -f ($osInfo.FreePhysicalMemory*1KB/1GB))} catch {}
    } catch {}
    try {
        $drive=[System.IO.DriveInfo]::new($env:SystemDrive)
        if($drive.IsReady){$free=Format-Bytes ([long]$drive.AvailableFreeSpace);$diskDetail='livres de '+(Format-Bytes ([long]$drive.TotalSize))}
    } catch {}
    [pscustomobject]@{WindowsName=$name;WindowsVersion=$version;Processor=$cpu;Memory=$memory;MemoryDetail=$memoryDetail;DiskFree=$free;DiskDetail=$diskDetail}
}

function Get-SafeTempRoot {
    if ([string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) { throw 'LOCALAPPDATA nao esta definida.' }
    $root=[System.IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'Temp')).TrimEnd('\')
    if($root.Length -lt 12 -or $root -eq [System.IO.Path]::GetPathRoot($root).TrimEnd('\')){throw 'Caminho temporario invalido.'}
    if(Test-Path -LiteralPath $root -PathType Container){
        $attr=[System.IO.File]::GetAttributes($root)
        if(($attr -band [System.IO.FileAttributes]::ReparsePoint) -ne 0){throw 'A pasta temporaria e um link ou junction. Limpeza bloqueada.'}
    }
    return $root
}
function Measure-DirectorySafe([string]$Path) {
    [long]$total=0
    try {
        foreach($entry in [System.IO.Directory]::EnumerateFileSystemEntries($Path)){
            try {
                $attr=[System.IO.File]::GetAttributes($entry)
                if(($attr -band [System.IO.FileAttributes]::ReparsePoint) -ne 0){continue}
                if(($attr -band [System.IO.FileAttributes]::Directory) -ne 0){$total += Measure-DirectorySafe $entry}
                else {$total += ([System.IO.FileInfo]::new($entry)).Length}
            } catch {}
        }
    } catch {}
    return $total
}
function Remove-TempEntrySafe([string]$Path,[hashtable]$Result) {
    try {
        $attr=[System.IO.File]::GetAttributes($Path)
        if(($attr -band [System.IO.FileAttributes]::ReparsePoint) -ne 0){$Result.Skipped++;return}
        if(($attr -band [System.IO.FileAttributes]::Directory) -ne 0){
            foreach($child in [System.IO.Directory]::EnumerateFileSystemEntries($Path)){Remove-TempEntrySafe $child $Result}
            try {[System.IO.Directory]::Delete($Path,$false);$Result.Directories++} catch {$Result.Skipped++}
        } else {
            $length=([System.IO.FileInfo]::new($Path)).Length
            try {Remove-Item -LiteralPath $Path -Force -ErrorAction Stop;$Result.Bytes += $length;$Result.Files++} catch {$Result.Skipped++}
        }
    } catch {$Result.Skipped++}
}
function Invoke-TempCleanup {
    $root=Get-SafeTempRoot
    if(-not(Test-Path -LiteralPath $root -PathType Container)){return [pscustomobject]@{Bytes=0L;Files=0;Directories=0;Skipped=0}}
    $r=@{Bytes=[long]0;Files=0;Directories=0;Skipped=0}
    foreach($entry in [System.IO.Directory]::EnumerateFileSystemEntries($root)){Remove-TempEntrySafe $entry $r}
    [pscustomobject]@{Bytes=[long]$r.Bytes;Files=[int]$r.Files;Directories=[int]$r.Directories;Skipped=[int]$r.Skipped}
}
function Get-WingetCommand {
    $cmd=Get-Command winget.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if(-not $cmd){$cmd=Get-Command winget -ErrorAction SilentlyContinue | Select-Object -First 1}
    return $cmd
}

function New-Label([string]$Text,[int]$X,[int]$Y,[int]$W,[int]$H,[System.Drawing.Color]$Color=$script:T.Text,[System.Drawing.Font]$Font=$script:F.Body) {
    $l=New-Object System.Windows.Forms.Label
    $l.Text=$Text; $l.Location=[System.Drawing.Point]::new($X,$Y); $l.Size=[System.Drawing.Size]::new($W,$H)
    $l.ForeColor=$Color; $l.BackColor=[System.Drawing.Color]::Transparent; $l.Font=$Font
    $l.TextAlign=[System.Drawing.ContentAlignment]::MiddleLeft; $l.AutoEllipsis=$true
    return $l
}
function New-Button([string]$Text,[int]$X,[int]$Y,[int]$W,[int]$H,[scriptblock]$Action,[bool]$Primary=$false,[bool]$Disabled=$false) {
    $b=New-Object System.Windows.Forms.Button
    $b.Text=$Text; $b.Location=[System.Drawing.Point]::new($X,$Y); $b.Size=[System.Drawing.Size]::new($W,$H)
    $b.FlatStyle=[System.System.Windows.Forms.FlatStyle]::Flat; $b.FlatAppearance.BorderSize=1
    $b.FlatAppearance.BorderColor=$script:T.Border; $b.FlatAppearance.MouseOverBackColor=$script:T.CardAlt
    $b.BackColor=if($Primary){$script:T.Accent}else{$script:T.CardAlt}
    $b.ForeColor=$script:T.Text; $b.Font=$script:F.Button; $b.Cursor=[System.System.Windows.Forms.Cursors]::Hand
    $b.Enabled=-not $Disabled
    if($Disabled){$b.BackColor=[System.Drawing.Color]::FromArgb(43,51,65);$b.ForeColor=$script:T.Muted}
    if($Action){$b.Add_Click($Action)}
    return $b
}
function New-Card([System.System.Windows.Forms.Control]$Parent,[int]$X,[int]$Y,[int]$W,[int]$H,[System.Drawing.Color]$Color=$script:T.Card) {
    $p=New-Object System.Windows.Forms.Panel
    $p.Location=[System.Drawing.Point]::new($X,$Y);$p.Size=[System.Drawing.Size]::new($W,$H)
    $p.BackColor=$Color;$p.BorderStyle=[System.System.Windows.Forms.BorderStyle]::FixedSingle
    $p.Anchor=[System.System.Windows.Forms.AnchorStyles]::Top -bor [System.System.Windows.Forms.AnchorStyles]::Left -bor [System.System.Windows.Forms.AnchorStyles]::Right
    $Parent.Controls.Add($p);return $p
}
function New-Page {
    $p=New-Object System.Windows.Forms.Panel;$p.Dock=[System.System.Windows.Forms.DockStyle]::Fill
    $p.AutoScroll=$true;$p.BackColor=$script:T.Bg;$script:Content.Controls.Add($p);return $p
}
function Add-Header([System.System.Windows.Forms.Control]$Page,[string]$Title,[string]$Subtitle) {
    $Page.Controls.Add((New-Label 'OTIMIZADOR  /  WINDOWS 10 • 11' 28 15 430 22 $script:T.Muted $script:F.Small))
    $Page.Controls.Add((New-Label $Title 28 38 790 42 $script:T.Text $script:F.Title))
    $Page.Controls.Add((New-Label $Subtitle 30 82 850 25 $script:T.Muted $script:F.Body))
}
function Set-NavStyles {
    foreach($key in $script:Nav.Keys){
        $b=$script:Nav[$key]
        if($key -eq $script:PageName){$b.BackColor=$script:T.AccentDark;$b.ForeColor=$script:T.Text;$b.FlatAppearance.BorderColor=$script:T.AccentDark}
        else {$b.BackColor=$script:T.Sidebar;$b.ForeColor=$script:T.Muted;$b.FlatAppearance.BorderColor=$script:T.Sidebar}
    }
}
function Add-NavButton([string]$Name,[string]$Caption,[int]$Y) {
    $b=New-Object System.Windows.Forms.Button
    $b.Text=$Caption;$b.Location=[System.Drawing.Point]::new(14,$Y);$b.Size=[System.Drawing.Size]::new(192,42)
    $b.FlatStyle=[System.System.Windows.Forms.FlatStyle]::Flat;$b.FlatAppearance.BorderSize=0
    $b.BackColor=$script:T.Sidebar;$b.ForeColor=$script:T.Muted;$b.Font=$script:F.Button
    $b.TextAlign=[System.Drawing.ContentAlignment]::MiddleLeft;$b.Padding=[System.System.Windows.Forms.Padding]::new(12,0,0,0)
    $b.Cursor=[System.System.Windows.Forms.Cursors]::Hand
    $target=$Name
    $navAction={Show-Page $target}.GetNewClosure()
    $b.Add_Click($navAction)
    $script:Nav[$Name]=$b;$script:Sidebar.Controls.Add($b)
}
function Show-Page([string]$Name) {
    $script:PageName=$Name;$script:Content.Controls.Clear();Set-NavStyles
    switch($Name){
        'Home' {New-DashboardPage | Out-Null}
        'Cleaner' {New-CleanerPage | Out-Null}
        'Debloat' {New-DebloatPage | Out-Null}
        'Apps' {New-AppsPage | Out-Null}
        'Tools' {New-ToolsPage | Out-Null}
        default {New-DashboardPage | Out-Null}
    }
    Set-Status 'Pronto. Escolha uma acao para comecar.'
}

function New-DashboardPage {
    $page=New-Page;Add-Header $page 'Seu Windows, mais leve.' 'Um painel simples para organizar o PC depois de uma formatacao.'
    $w=[Math]::Max(680,$page.ClientSize.Width-56)
    $hero=New-Card $page 28 120 $w 158 $script:T.CardAlt
    $hero.Controls.Add((New-Label 'PRONTO PARA COMECAR?' 24 16 340 22 $script:T.Muted $script:F.Small))
    $hero.Controls.Add((New-Label 'Cuide do seu PC em poucos passos.' 24 42 ($w-60) 40 $script:T.Text $script:F.Heading))
    $hero.Controls.Add((New-Label 'Limpe temporarios, revise aplicativos opcionais e instale ferramentas conhecidas.' 26 82 ($w-60) 32 $script:T.Muted $script:F.Body))
    $hero.Controls.Add((New-Button 'Abrir limpeza' 26 119 160 30 {Show-Page 'Cleaner'} $true))
    $hero.Controls.Add((New-Button 'Revisar aplicativos' 196 119 180 30 {Show-Page 'Debloat'} $false))

    $gap=14;$cw=[int][Math]::Floor(($w-2*$gap)/3);$s=Get-SystemSnapshot
    $items=@(
        [pscustomobject]@{X=28;Title='SISTEMA OPERACIONAL';Value=$s.WindowsName;Detail=$s.WindowsVersion},
        [pscustomobject]@{X=(28+$cw+$gap);Title='MEMORIA INSTALADA';Value=$s.Memory;Detail=$s.MemoryDetail},
        [pscustomobject]@{X=(28+2*($cw+$gap));Title='ESPACO LIVRE';Value=$s.DiskFree;Detail=$s.DiskDetail}
    )
    foreach($item in $items){
        $c=New-Card $page $item.X 300 $cw 126
        $c.Controls.Add((New-Label $item.Title 16 14 ($cw-28) 20 $script:T.Muted $script:F.Small))
        $c.Controls.Add((New-Label $item.Value 16 40 ($cw-28) 38 $script:T.Text $script:F.Title))
        $c.Controls.Add((New-Label $item.Detail 16 84 ($cw-28) 26 $script:T.Muted $script:F.Small))
    }
    $cpu=New-Card $page 28 444 $w 76
    $cpu.Controls.Add((New-Label 'PROCESSADOR DETECTADO' 18 8 ($w-36) 20 $script:T.Muted $script:F.Small))
    $cpu.Controls.Add((New-Label $s.Processor 18 31 ($w-36) 30 $script:T.Text $script:F.Body))
    $page.Controls.Add((New-Label 'FLUXO RECOMENDADO' 28 540 $w 22 $script:T.Muted $script:F.Small))
    $half=[int][Math]::Floor(($w-14)/2)
    $a=New-Card $page 28 568 $half 82;$b=New-Card $page (42+$half) 568 $half 82
    $a.Controls.Add((New-Label '01  •  Arquivos temporarios' 14 8 ($half-28) 24 $script:T.Text $script:F.Button))
    $a.Controls.Add((New-Label 'Libere espaco sem tocar nas pastas do sistema.' 14 37 ($half-28) 25 $script:T.Muted $script:F.Small))
    $b.Controls.Add((New-Label '02  •  Aplicativos opcionais' 14 8 ($half-28) 24 $script:T.Text $script:F.Button))
    $b.Controls.Add((New-Label 'Escolha manualmente o que realmente nao usa.' 14 37 ($half-28) 25 $script:T.Muted $script:F.Small))
}

function New-CleanerPage {
    $page=New-Page;Add-Header $page 'Limpeza inteligente' 'Confira o que sera removido antes de executar cada acao.'
    $w=[Math]::Max(680,$page.ClientSize.Width-56);$gap=16;$half=[int][Math]::Floor(($w-$gap)/2);$rx=28+$half+$gap
    $temp=New-Card $page 28 122 $half 300
    $temp.Controls.Add((New-Label 'ARQUIVOS TEMPORARIOS' 20 20 ($half-40) 22 $script:T.Muted $script:F.Small))
    $script:TempSizeLabel=New-Label (Format-Bytes (Measure-DirectorySafe (Get-SafeTempRoot))) 20 62 ($half-40) 48 $script:T.Text $script:F.Title
    $temp.Controls.Add($script:TempSizeLabel)
    $temp.Controls.Add((New-Label 'Tamanho estimado em LOCALAPPDATA\Temp' 20 116 ($half-40) 24 $script:T.Muted $script:F.Small))
    $temp.Controls.Add((New-Label 'Arquivos em uso, protegidos e links simbolicos serao ignorados.' 20 155 ($half-40) 48 $script:T.Muted $script:F.Body))
    $temp.Controls.Add((New-Button 'Atualizar tamanho' 20 218 145 34 {
        try{$script:TempSizeLabel.Text=Format-Bytes (Measure-DirectorySafe (Get-SafeTempRoot));Set-Status 'Tamanho atualizado.'}catch{Show-Warning $_.Exception.Message}
    } $false))
    $temp.Controls.Add((New-Button 'Limpar temporarios' 177 218 160 34 {
        try {
            if(-not(Confirm-Action 'Remover os arquivos encontrados em LOCALAPPDATA\Temp? Arquivos em uso e links simbolicos serao ignorados.')){return}
            $r=Invoke-TempCleanup
            $script:TempSizeLabel.Text=Format-Bytes (Measure-DirectorySafe (Get-SafeTempRoot))
            $msg=@('Limpeza concluida.','',"Espaco liberado: $(Format-Bytes $r.Bytes)","Arquivos removidos: $($r.Files)","Pastas removidas: $($r.Directories)","Itens ignorados: $($r.Skipped)") -join [Environment]::NewLine
            Set-Status "Limpeza concluida. Espaco liberado: $(Format-Bytes $r.Bytes)";Show-Info $msg
        }catch{Show-Warning "Falha na limpeza: $($_.Exception.Message)"}
    } $true))

    $bin=New-Card $page $rx 122 $half 300
    $bin.Controls.Add((New-Label 'LIXEIRA DO WINDOWS' 20 20 ($half-40) 22 $script:T.Muted $script:F.Small))
    $bin.Controls.Add((New-Label 'Recupere espaco' 20 62 ($half-40) 48 $script:T.Text $script:F.Title))
    $bin.Controls.Add((New-Label 'Os itens da Lixeira serao removidos permanentemente. Confira antes de continuar.' 20 124 ($half-40) 65 $script:T.Muted $script:F.Body))
    $bin.Controls.Add((New-Label 'ACAO PERMANENTE • EXIGE CONFIRMACAO' 20 198 ($half-40) 24 $script:T.Orange $script:F.Small))
    $bin.Controls.Add((New-Button 'Esvaziar Lixeira' 20 218 170 34 {
        if(-not(Confirm-Action 'Esvaziar a Lixeira e uma acao permanente. Deseja continuar?')){return}
        try{Clear-RecycleBin -Force -ErrorAction Stop;Set-Status 'Lixeira esvaziada.';Show-Info 'A Lixeira foi esvaziada.'}catch{Show-Warning "Nao foi possivel esvaziar a Lixeira: $($_.Exception.Message)"}
    } $false))
    $note=New-Card $page 28 442 $w 82
    $note.Controls.Add((New-Label 'ESTADO E SEGURANCA' 18 9 ($w-36) 20 $script:T.Muted $script:F.Small))
    $note.Controls.Add((New-Label 'O otimizador nao apaga pastas do Windows, Prefetch, drivers ou arquivos pessoais.' 18 35 ($w-36) 28 $script:T.Text $script:F.Body))
}

function Update-DebloatCounter {
    if($script:DebloatCount){$n=@($script:DebloatItems | Where-Object {$_.Check.Checked}).Count;$script:DebloatCount.Text="$n selecionado(s)"}
}
function New-DebloatPage {
    $page=New-Page;Add-Header $page 'Debloat seletivo' 'Voce escolhe. Nenhum aplicativo e removido sem marcar e confirmar.'
    $w=[Math]::Max(680,$page.ClientSize.Width-56);$script:DebloatItems=@()
    $bar=New-Card $page 28 120 $w 40
    $bar.Controls.Add((New-Label 'APLICATIVOS OPCIONAIS' 16 7 300 24 $script:T.Muted $script:F.Small))
    $script:DebloatCount=New-Label '0 selecionado(s)' ($w-190) 7 170 24 $script:T.Accent $script:F.Button
    $script:DebloatCount.TextAlign=[System.Drawing.ContentAlignment]::MiddleRight;$bar.Controls.Add($script:DebloatCount)
    for($i=0;$i -lt $script:OptionalApps.Count;$i++){
        $app=$script:OptionalApps[$i];$y=174+($i*47);$row=New-Card $page 28 $y $w 42
        $check=New-Object System.Windows.Forms.CheckBox;$check.Location=[System.Drawing.Point]::new(14,8);$check.Size=[System.Drawing.Size]::new(22,24)
        $check.Text='';$check.BackColor=[System.Drawing.Color]::Transparent;$check.FlatStyle=[System.System.Windows.Forms.FlatStyle]::Flat
        $changedAction={Update-DebloatCounter}.GetNewClosure()
        $check.Add_CheckedChanged($changedAction);$row.Controls.Add($check)
        $row.Controls.Add((New-Label $app.Name 48 1 ($w-330) 20 $script:T.Text $script:F.Button))
        $row.Controls.Add((New-Label $app.Description 48 20 ($w-330) 18 $script:T.Muted $script:F.Small))
        $script:DebloatItems += [pscustomobject]@{App=$app;Check=$check}
    }
    $remove=New-Button 'Remover selecionados' 28 560 210 38 {
        $selected=@($script:DebloatItems | Where-Object {$_.Check.Checked})
        if($selected.Count -eq 0){Show-Info 'Marque pelo menos um aplicativo opcional.';return}
        $names=($selected | ForEach-Object {'- '+$_.App.Name}) -join [Environment]::NewLine
        $question=@('Remover estes aplicativos apenas do usuario atual?','',$names,'','A reinstalacao pode exigir a Microsoft Store.') -join [Environment]::NewLine
        if(-not(Confirm-Action $question 'Confirmar debloat')){return}
        $ok=0;$missing=0;$failed=New-Object System.Collections.Generic.List[string]
        foreach($entry in $selected){
            try{
                $matches=@(Get-AppxPackage -Name $entry.App.Id -ErrorAction SilentlyContinue | Where-Object {$_.Name -eq $entry.App.Id})
                if($matches.Count -eq 0){$missing++;$entry.Check.Checked=$false;continue}
                foreach($p in $matches){Remove-AppxPackage -Package $p.PackageFullName -ErrorAction Stop}
                $ok++
            }catch{$failed.Add("$($entry.App.Name): $($_.Exception.Message)")}
            $entry.Check.Checked=$false
        }
        Update-DebloatCounter
        $report="Removidos: $ok  |  Nao instalados: $missing  |  Falhas: $($failed.Count)"
        if($failed.Count -gt 0){$report+=[Environment]::NewLine+($failed -join [Environment]::NewLine);Show-Warning $report}else{Show-Info $report}
        Set-Status "Debloat finalizado. Removidos: $ok"
    } $true
    $page.Controls.Add($remove)
    $page.Controls.Add((New-Label 'A remocao afeta somente o usuario atual. Store, Defender, Edge e componentes essenciais nao estao na lista.' 254 560 ($w-270) 38 $script:T.Muted $script:F.Small))
}

function New-AppsPage {
    $page=New-Page;Add-Header $page 'Biblioteca de aplicativos' 'Instale ferramentas conhecidas pelo Windows Package Manager (winget).'
    $w=[Math]::Max(680,$page.ClientSize.Width-56);$cmd=Get-WingetCommand;$ready=[bool]$cmd
    $statusColor=if($ready){[System.Drawing.Color]::FromArgb(16,43,37)}else{[System.Drawing.Color]::FromArgb(49,36,24)}
    $statusText=if($ready){'WINGET DISPONIVEL • pronto para instalar'}else{'WINGET NAO ENCONTRADO • atualize o App Installer pela Microsoft Store'}
    $statusTextColor=if($ready){$script:T.Green}else{$script:T.Orange}
    $status=New-Card $page 28 120 $w 44 $statusColor
    $status.Controls.Add((New-Label $statusText 16 8 ($w-32) 24 $statusTextColor $script:F.Button))
    $gap=14;$cw=[int][Math]::Floor(($w-$gap)/2)
    for($i=0;$i -lt $script:Packages.Count;$i++){
        $pkg=$script:Packages[$i];$col=$i%2;$row=[Math]::Floor($i/2);$x=28+($col*($cw+$gap));$y=178+($row*74)
        $card=New-Card $page $x $y $cw 64
        $card.Controls.Add((New-Label $pkg.Name 14 7 ($cw-135) 22 $script:T.Text $script:F.Button))
        $card.Controls.Add((New-Label $pkg.Description 14 32 ($cw-135) 20 $script:T.Muted $script:F.Small))
        $selectedPackage=$pkg
        $installAction = {
            try {
                $winget=Get-WingetCommand
                if(-not $winget){
                    Start-Process 'ms-windows-store://pdp/?ProductId=9NBLGGH4NNS1'
                    Show-Warning 'Atualize o App Installer pela Microsoft Store para obter winget.'
                    return
                }
                $args='install --id "{0}" -e --source winget' -f $selectedPackage.Id
                Start-Process -FilePath $winget.Source -ArgumentList $args -WindowStyle Normal
                Set-Status "Instalador iniciado: $($selectedPackage.Name). Acompanhe o terminal."
            } catch { Show-Warning "Nao foi possivel iniciar: $($_.Exception.Message)" }
        }.GetNewClosure()
        $button=New-Button 'Instalar' ($cw-106) 15 88 34 $installAction $ready
        $card.Controls.Add($button)
    }
}

function New-ToolsPage {
    $page=New-Page;Add-Header $page 'Ferramentas extras' 'Atalhos para configuracoes do Windows e projetos da comunidade.'
    $w=[Math]::Max(680,$page.ClientSize.Width-56);$half=[int][Math]::Floor(($w-14)/2);$rx=42+$half
    $a=New-Card $page 28 130 $half 140
    $a.Controls.Add((New-Label '01  /  INICIALIZACAO' 18 14 ($half-36) 22 $script:T.Muted $script:F.Small))
    $a.Controls.Add((New-Label 'Controle o que inicia' 18 43 ($half-36) 28 $script:T.Text $script:F.Heading))
    $a.Controls.Add((New-Label 'Revise os aplicativos que iniciam junto com o Windows.' 18 76 ($half-36) 36 $script:T.Muted $script:F.Body))
    $a.Controls.Add((New-Button 'Abrir configuracoes' 18 106 170 28 {Start-Process 'ms-settings:startupapps'} $true))
    $b=New-Card $page $rx 130 $half 140
    $b.Controls.Add((New-Label '02  /  PROGRAMAS' 18 14 ($half-36) 22 $script:T.Muted $script:F.Small))
    $b.Controls.Add((New-Label 'Aplicativos instalados' 18 43 ($half-36) 28 $script:T.Text $script:F.Heading))
    $b.Controls.Add((New-Label 'Use a tela oficial do Windows para alterar ou remover programas.' 18 76 ($half-36) 36 $script:T.Muted $script:F.Body))
    $b.Controls.Add((New-Button 'Abrir aplicativos' 18 106 170 28 {Start-Process 'ms-settings:appsfeatures'} $false))
    $c=New-Card $page 28 290 $half 140
    $c.Controls.Add((New-Label '03  /  SEGURANCA' 18 14 ($half-36) 22 $script:T.Muted $script:F.Small))
    $c.Controls.Add((New-Label 'Seguranca do Windows' 18 43 ($half-36) 28 $script:T.Text $script:F.Heading))
    $c.Controls.Add((New-Label 'Abra o painel oficial para revisar protecao e firewall.' 18 76 ($half-36) 36 $script:T.Muted $script:F.Body))
    $c.Controls.Add((New-Button 'Abrir seguranca' 18 106 170 28 {Start-Process 'windowsdefender:'} $true))
    $d=New-Card $page $rx 290 $half 140
    $d.Controls.Add((New-Label '04  /  ATUALIZACOES' 18 14 ($half-36) 22 $script:T.Muted $script:F.Small))
    $d.Controls.Add((New-Label 'Windows Update' 18 43 ($half-36) 28 $script:T.Text $script:F.Heading))
    $d.Controls.Add((New-Label 'Confira atualizacoes e reinicializacoes pendentes.' 18 76 ($half-36) 36 $script:T.Muted $script:F.Body))
    $d.Controls.Add((New-Button 'Abrir atualizacoes' 18 106 170 28 {Start-Process 'ms-settings:windowsupdate'} $false))
    $community=New-Card $page 28 450 $w 106
    $community.Controls.Add((New-Label 'PROJETOS DA COMUNIDADE' 18 10 ($w-36) 22 $script:T.Muted $script:F.Small))
    $community.Controls.Add((New-Label 'SpotifyLite e DiscordDebloat' 18 38 360 25 $script:T.Text $script:F.Button))
    $community.Controls.Add((New-Label 'Projetos externos. Leia as notas do release e valide os arquivos antes de executar.' 18 64 ($w-390) 30 $script:T.Muted $script:F.Small))
    $community.Controls.Add((New-Button 'Ver releases' ($w-168) 48 142 34 {
        Start-Process 'https://github.com/TeamAntiLag/SpotifyLite/releases/tag/SpotifyLite'
        Start-Process 'https://github.com/TeamAntiLag/DiscordDebloat/releases/tag/v2.1'
    } $false))
}

# Montagem da janela principal
$script:Form=New-Object System.Windows.Forms.Form
$script:Form.Text='Otimizador - Windows 10 / 11'
$script:Form.StartPosition=[System.System.Windows.Forms.FormStartPosition]::CenterScreen
$script:Form.Size=[System.Drawing.Size]::new(1240,820)
$script:Form.MinimumSize=[System.Drawing.Size]::new(1080,720)
$script:Form.BackColor=$script:T.Bg;$script:Form.ForeColor=$script:T.Text;$script:Form.Font=$script:F.Body

$footer=New-Object System.Windows.Forms.Panel
$footer.Dock=[System.System.Windows.Forms.DockStyle]::Bottom;$footer.Height=30;$footer.BackColor=$script:T.Sidebar
$script:Status=New-Label '  Pronto. Escolha uma acao para comecar.' 8 2 950 24 $script:T.Muted $script:F.Small
$footer.Controls.Add($script:Status)

$script:Content=New-Object System.Windows.Forms.Panel
$script:Content.Dock=[System.System.Windows.Forms.DockStyle]::Fill;$script:Content.BackColor=$script:T.Bg

$script:Sidebar=New-Object System.Windows.Forms.Panel
$script:Sidebar.Dock=[System.System.Windows.Forms.DockStyle]::Left;$script:Sidebar.Width=220;$script:Sidebar.BackColor=$script:T.Sidebar
$brand=New-Label 'O' 22 22 42 42 ([System.Drawing.Color]::White) $script:F.Title
$brand.BackColor=$script:T.Accent;$brand.TextAlign=[System.Drawing.ContentAlignment]::MiddleCenter
$script:Sidebar.Controls.Add($brand)
$script:Sidebar.Controls.Add((New-Label 'OTIMIZADOR' 76 22 125 24 $script:T.Text $script:F.Button))
$script:Sidebar.Controls.Add((New-Label 'PC CARE  •  DESKTOP' 77 46 125 20 $script:T.Muted $script:F.Small))
$line=New-Object System.Windows.Forms.Panel;$line.Location=[System.Drawing.Point]::new(20,82);$line.Size=[System.Drawing.Size]::new(178,1);$line.BackColor=$script:T.Border
$script:Sidebar.Controls.Add($line)
$script:Sidebar.Controls.Add((New-Label 'ESPACO DE TRABALHO' 24 98 170 22 $script:T.Muted $script:F.Small))
Add-NavButton 'Home' '  Visao geral' 132
Add-NavButton 'Cleaner' '  Limpeza' 180
Add-NavButton 'Debloat' '  Debloat seletivo' 228
Add-NavButton 'Apps' '  Aplicativos' 276
Add-NavButton 'Tools' '  Ferramentas' 324
$line2=New-Object System.Windows.Forms.Panel;$line2.Location=[System.Drawing.Point]::new(20,392);$line2.Size=[System.Drawing.Size]::new(178,1);$line2.BackColor=$script:T.Border
$script:Sidebar.Controls.Add($line2)
$script:Sidebar.Controls.Add((New-Label 'SEGURO POR PADRAO' 24 414 170 22 $script:T.Green $script:F.Small))
$script:Sidebar.Controls.Add((New-Label 'Acoes seletivas. Defender, firewall e servicos essenciais permanecem intactos.' 24 440 170 72 $script:T.Muted $script:F.Small))

$script:Form.Controls.Add($script:Content)
$script:Form.Controls.Add($script:Sidebar)
$script:Form.Controls.Add($footer)

Show-Page 'Home'
[void]$script:Form.ShowDialog()
foreach($font in $script:F.Values){if($font){$font.Dispose()}}
