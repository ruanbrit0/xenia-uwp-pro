# Xbox 360 Architecture Optimization Notes

Este documento resume informacoes publicas da analise "Xbox 360
Architecture", de Rodrigo Copetti, e traduz os pontos mais uteis para o foco
deste fork: melhorar compatibilidade e performance no UWP/Xbox.

Fonte principal:
https://www.copetti.org/writings/consoles/xbox-360/

## Uso Correto

Use estas notas como mapa de investigacao, nao como prova final de
implementacao. Antes de mudar codigo, validar com logs, traces, testes PPC/GPU,
comportamento observado no jogo e comparacao com o baseline UWP deste fork.

As informacoes abaixo sao especialmente uteis para priorizar onde medir:
- GPU D3D12, EDRAM, tiled rendering, resolves e memexport.
- CPU/JIT, VMX128, instrucoes de cache, waits e scheduler.
- APU/XMA, streaming, offsets, buffers e filas.
- VFS/XContent/SVOD/STFS, I/O assincrono e loading.
- Configuracao UWP/Xbox, principalmente resolucao e readbacks.

## Prioridades Praticas

### 1. EDRAM, Tiles E Resolves

O Xbox 360 real usa uma GPU Xenos com `10 MB` de EDRAM rapida para render
targets, Z, stencil, backbuffer, MSAA e operacoes de pixel. Esse tamanho e
pequeno para HD com MSAA, entao jogos dependem de tiled rendering, resolves e
reconstrucao correta dos buffers finais.

Impacto no emulador:
- Problemas visuais devem priorizar `render_target_path_d3d12`, resolves,
  copias de render target, MRT, tiling e sincronizacao de EDRAM.
- `d3d12_readback_resolve = true` faz sentido para casos concretos de
  render-to-texture, texto/menu invisivel e efeitos ausentes.
- Readback de resolve nao deve virar padrao global porque sincroniza GPU e CPU
  no meio do frame e pode derrubar FPS no Xbox/UWP.

### 2. Evitar Readbacks Globais

O hardware real tem caminhos dedicados para CPU/GPU e coerencia, incluindo
streaming para a GPU e escrita de status de volta para CPU. No D3D12 host, um
readback costuma ser muito mais caro porque pode forcar flush/sincronizacao do
pipeline.

Impacto no emulador:
- Manter `d3d12_readback_resolve = false` por padrao.
- Manter `d3d12_readback_memexport = false` por padrao.
- Ativar por jogo somente quando corrigir sintoma real e documentado.
- Medir FPS e stutter antes/depois, sempre sem logs pesados.

### 3. Memexport E Shader Export

Xenos permite que shaders exportem dados para memoria principal. Alguns jogos
podem usar isso como computacao ou como parte de efeitos/render setup.

Impacto no emulador:
- `d3d12_readback_memexport` pode corrigir jogos que dependem de dados escritos
  pela GPU e lidos pelo CPU.
- A otimizacao ideal e detectar quando o CPU realmente consome esses dados, nao
  sincronizar sempre.
- Se um jogo melhora com memexport, registrar fase, sintoma, log e custo de FPS.

### 4. Baseline 720p

O artigo reforca que muitos jogos do Xbox 360 renderizavam mirando 1280x720 e
usavam upscale para saidas maiores. 1080p real aumenta custo de pixel, MSAA,
EDRAM, bandwidth e resolves.

Impacto no emulador:
- Usar `internal_display_resolution = 8` como baseline UWP/Xbox.
- Para FPS baixo, testar `internal_display_resolution = 0` antes de alterar
  codigo, separando gargalo de GPU de gargalo de CPU/APU/VFS.
- Evitar escala de render acima de 1x em debug inicial de problema visual.

### 5. CPU Xenon, Threads E Scheduler

Xenon tem tres nucleos PowerPC a `3.2 GHz`, cada um com dois hardware threads.
Como os nucleos sao in-order, jogos dependem bastante de paralelismo explicito,
waits, prioridades e afinidades.

Impacto no emulador:
- Bugs de loading/gameplay podem estar em waits, eventos, overlapped I/O,
  prioridades, afinidades ou scheduling, nao apenas em GPU.
- `ignore_thread_affinities` e `ignore_thread_priorities` devem continuar sendo
  avaliados por jogo e por host, porque o Xbox UWP atual nao replica Xenon.
- Alteracoes em threading/timing exigem reteste contra baseline Forza.

### 6. Cache Line, L2 E Instrucoes De Cache

Xenon usa L2 compartilhado de `1 MB`, linha de cache de `128 bytes` e associacao
8-way. O artigo tambem menciona custo alto de cache miss da CPU para RAM por
causa da arquitetura UMA.

Impacto no emulador:
- Revisar com cuidado instrucoes PPC de cache e sincronizacao como `dcbt`,
  `dcbz`, `dcbst`, `dcbf`, `sync`, `lwsync` e equivalentes ja emulados.
- Bugs de dados stale entre CPU/GPU podem depender de barreiras ou cache-control.
- Otimizacoes de memoria devem evitar invalidacoes globais se for possivel
  invalidar/faixar regioes menores.

### 7. VMX128

VMX128 adiciona registradores e instrucoes vetoriais especificas do Xbox 360,
incluindo dot product e operacoes relacionadas a formatos Direct3D.

Impacto no emulador:
- JIT/VMX128 e area de alto retorno em jogos CPU-bound.
- Erros aqui podem aparecer como fisica quebrada, animacao errada, decompressao
  falhando, audio instavel ou setup de render incorreto.
- Ao tocar VMX128, adicionar ou rodar testes PPC focados quando possivel.

### 8. XMA E APU

No console real, audio geral e software, mas XMA tem decoder dedicado no
Southbridge. O decoder le dados XMA da RAM, gera PCM em RAM e o CPU ainda pode
aplicar efeitos/streaming depois.

Impacto no emulador:
- XMA e ponto sensivel para stutter, loading infinito e audio atrasado.
- Priorizar filas, offsets, buffers, flush, quantidade de frames enfileirados e
  sincronizacao com o guest.
- Mudancas em thread dedicada de XMA no UWP devem ser experimentais e retestadas
  no Xbox real.

### 9. Southbridge, I/O E Storage

No Xbox 360, Southbridge concentra I/O e pode escrever em RAM com mecanismos de
coerencia de cache. Jogos foram feitos esperando DVD/HDD/USB, FATX, STFS,
XContent e operacoes assincronas eficientes.

Impacto no emulador:
- Loading e stutter podem vir de VFS/XContent/SVOD/STFS, nao apenas GPU.
- Reduzir locks longos e evitar seek/read concorrente inseguro continua
  importante no UWP.
- Para jogos em USB/disco externo no Xbox atual, medir diferenca entre gargalo de
  armazenamento e gargalo de emulacao.

### 10. XAM, Perfil E Saves

O sistema real usa XAM para perfil, saves, achievements e content device. Muitos
jogos bloqueiam em menu/loading quando o perfil ou storage nao responde como
esperado.

Impacto no emulador:
- Antes de tratar menu travado como bug de GPU, conferir XAM, perfil, save,
  device selector e content root.
- Melhorar stubs XAM pode aumentar compatibilidade sem custo grande de GPU.

## Informacoes Mais Acionaveis

| Area | O que aproveitar | Risco |
| --- | --- | --- |
| GPU D3D12 | EDRAM, resolves, tiling, MRT, render target paths | Alto impacto em FPS |
| Readbacks | Usar so por jogo quando corrigir sintoma real | Stutter forte no UWP |
| Memexport | Detectar consumo CPU antes de sincronizar | Regressao visual/timing |
| CPU/JIT | VMX128 e cache-control | Regressao ampla |
| Threading | Waits, affinities, priorities, scheduler | Regressao de compatibilidade |
| APU/XMA | Buffers, offsets, filas e decoder | Audio/loading quebrado |
| VFS/I/O | STFS/SVOD/XContent e streaming | Loading/stutter |
| XAM | Perfil, saves e content device | Travamento em menu |

## Anexo: Fontes Publicas E Uso Pratico

Este anexo lista informacoes publicas encontradas durante a pesquisa e como elas
devem orientar investigacoes neste fork. Ele nao deve ser tratado como
especificacao oficial do hardware; use como guia para escolher testes, flags e
areas de codigo a medir.

### Xenia GPU / Render Target Cache

Fonte principal:
https://xenia.jp/updates/2021/04/27/leaving-no-pixel-behind-new-render-target-cache-3x3-resolution-scaling.html

Pontos uteis:
- A EDRAM do Xbox 360 e pequena (`10 MB`) e e usada para render targets, depth,
  stencil, MSAA e backbuffer temporario.
- Jogos resolvem dados da EDRAM para memoria principal quando precisam apresentar
  a imagem, alimentar texturas ou reconstruir buffers em passes posteriores.
- Render targets podem compartilhar a mesma faixa de EDRAM em momentos
  diferentes do frame, inclusive com reinterpretacao de formato/layout.
- Texturas em memoria principal usam layout tiled; render-to-texture precisa
  preservar esse modelo para casos como resolves parciais, pitch arredondado e
  leitura posterior por shaders.
- Readbacks GPU->CPU no D3D12 quebram o paralelismo CPU/GPU porque introduzem
  sincronizacao no meio do frame.

Aplicacao no projeto:
- Investigar bugs visuais primeiro em `src/xenia/gpu/d3d12/`, especialmente
  render target cache, resolve, texture cache, memexport e shared memory.
- Manter `d3d12_readback_resolve` e `d3d12_readback_memexport` desligados por
  padrao; ativar por jogo apenas quando corrigirem sintoma documentado.
- Antes de alterar codigo para FPS baixo, comparar 720p e 480p e confirmar que
  logs pesados estao desligados.
- Em problemas de precisao de render target, testar `render_target_path_d3d12`
  como diagnostico: `rtv` tende a ser mais rapido e menos preciso; `rov` tende a
  ser mais preciso, mas pode ser muito mais lento e depende de suporte/driver.

### Free60 / Xenon CPU

Fonte:
https://free60.org/Hardware/Console/Xenon_(CPU)/

Pontos uteis:
- Xenon tem tres nucleos PowerPC simetricos a `3.2 GHz`, com dois hardware
  threads por nucleo, totalizando seis threads.
- Cada nucleo tem L1 de instrucao e dados de `32 KiB`.
- O L2 compartilhado tem `1 MB`, roda a meia frequencia e e relevante para
  comportamento de cache/coerencia.
- A CPU e in-order, entao jogos dependem bastante de paralelismo explicito,
  waits, prioridades, afinidades e bom uso de VMX128.
- Cada hardware thread tem `128` registradores VMX128.

Aplicacao no projeto:
- Tratar travamentos de loading/gameplay tambem como possiveis problemas de
  scheduler, waits, afinidade, prioridade, barreiras ou instrucoes de cache.
- Ao alterar JIT/VMX128, rodar testes PPC focados sempre que possivel.
- Validar mudancas globais de threading/timing contra o baseline UWP de Forza.

### Free60 / STFS

Fonte:
https://free60.org/System-Software/Formats/STFS/

Pontos uteis:
- STFS e usado em pacotes XContent como PIRS, LIVE e `CON `.
- O conteudo e organizado em blocos de `4096` bytes com tabelas de hash.
- Arquivos internos nem sempre estao em blocos consecutivos; a cadeia de blocos
  precisa ser respeitada.
- SVOD tem descritor proprio dentro do mesmo contexto de pacotes/conteudo.

Aplicacao no projeto:
- Loading lento ou stutter em GOD/STFS/SVOD pode ser causado por fragmentacao,
  muitas leituras pequenas, seeks e locks, nao apenas por GPU ou CPU.
- Manter leitura concorrente segura em XContent/SVOD; o lock por `FILE*` em
  `xcontent_container_file.cc` evita corrida entre `Seek` e `fread`.
- Para otimizar VFS, medir padroes reais de leitura antes de adicionar caches ou
  mudar granularidade de locks.

### Microsoft Learn / UWP Filesystem E Capabilities

Fontes:
- https://learn.microsoft.com/en-us/windows/uwp/files/file-access-permissions
- https://learn.microsoft.com/en-us/windows/uwp/packaging/app-capability-declarations

Pontos uteis:
- O diretorio instalado do pacote e somente leitura.
- Dados mutaveis do app devem ir para `ApplicationData.LocalFolder`, que neste
  fork corresponde ao uso pratico de `LocalState`.
- Acesso persistente a arquivos fora do sandbox depende de picker,
  `FutureAccessList`, associacoes de tipo ou capabilities.
- `broadFileSystemAccess` e uma capability restrita e a propria documentacao da
  Microsoft marca essa capability como nao suportada no Xbox.
- Capabilities restritas podem exigir aprovacao no Store/Partner Center; em
  sideload isso e diferente, mas ainda pode afetar deploy em ambientes fechados.

Aplicacao no projeto:
- Configs padrao empacotadas devem ser lidas do install location e copiadas para
  `LocalState/config/` sem sobrescrever arquivos do usuario.
- Nao depender de `broadFileSystemAccess` como solucao para Xbox; preferir fluxos
  compativeis com sandbox, picker, pastas locais e conteudo explicitamente
  escolhido/montado.
- Manter `runFullTrust` fora do manifesto UWP enquanto nao houver necessidade
  concreta e validada.

### MultimediaWiki / XMA

Fonte:
https://wiki.multimedia.cx/index.php/XMA

Pontos uteis:
- XMA e o formato de audio usado no Xbox 360 e e descrito publicamente como
  baseado em WMA Pro.
- A documentacao publica e limitada; ela serve como contexto de codec/container,
  nao como especificacao completa de decoder.

Aplicacao no projeto:
- Para audio atrasado, stutter ou loading infinito, priorizar logs de contexto
  XMA, offsets, packet/frame counts, flush e filas.
- No UWP, manter `use_new_decoder = false` e `use_dedicated_xma_thread = false`
  por padrao ate haver validacao real em Xbox.

### Ordem Recomendada Para Diagnostico Por Jogo

1. Confirmar Title ID, midia usada, config local ativa e sintoma reproduzivel.
2. Desligar logs pesados: `flush_log`, `log_fps`, `log_draw_stats` e
   `log_viz_query_stats`.
3. Testar baseline 720p (`internal_display_resolution = 8`).
4. Se o problema for FPS/stutter, comparar com 480p
   (`internal_display_resolution = 0`).
5. Se o problema for visual, testar `d3d12_readback_resolve = true` isoladamente.
6. Se houver suspeita de dados GPU exportados para CPU, testar
   `d3d12_readback_memexport = true` isoladamente.
7. Se houver suspeita de precisao de render target, testar
   `render_target_path_d3d12 = "rov"` quando suportado.
8. Se uma flag corrigir o jogo, mover para config por jogo e documentar custo de
   FPS, fase afetada e sintoma corrigido.
9. Se nenhuma flag corrigir, coletar log/trace e investigar a area indicada:
   GPU/EDRAM, CPU/JIT, APU/XMA, VFS/STFS/SVOD ou XAM/storage.

## Inventario Das Imagens

As imagens do artigo foram revisadas pelo Markdown/HTML, URLs, legendas e
contexto. Elas nao devem ser copiadas para o repo; use apenas como referencia
visual externa.

| Categoria | Conteudo | Utilidade para otimizacao |
| --- | --- | --- |
| Modelos e motherboard | Revisoes fisicas, placa e diagrama geral | Baixa/media |
| CPU | Xenon, XBAR, cache, PPE, memoria e threading | Alta |
| GPU | Xenos, pipelines, comandos, vertex, raster, pixel e post | Alta |
| EDRAM/memoria | Organizacao de GDDR3, EDRAM e buffers | Alta |
| Audio | Pipeline XMA e contexto WMA | Media |
| I/O | Southbridge, portas, USB, controle e Kinect | Media |
| OS/storage | Privilegios, NAND, HDD, Memory Unit, USB e dashboards | Media |
| Games/rede | Disco, loja, HD-DVD e achievements | Baixa/media |
| Seguranca/homebrew | Crypto, exploits, XeLL, RGH e revisoes finais | Baixa |

## Onde Usar No Projeto

- `docs/gpu.md`: Xenos, EDRAM, tiled rendering, resolves, MRT e memexport.
- `docs/cpu.md`: Xenon, VMX128, L2, cache line de 128 bytes e `xdcbt`.
- `docs/uwp.md`: baseline 720p, readbacks por jogo e perfil Xbox/UWP.
- `docs/testing.md`: checklist de validacao para FPS, visual, XMA, VFS e XAM.
- `debug.md`: heuristicas praticas durante investigacao de jogos.

## Caveats

- Fonte publica secundaria nao substitui trace, log ou teste automatizado.
- Numeros de hardware ajudam a formular hipoteses, nao timing ciclo-a-ciclo.
- Evitar hacks especificos por jogo quando uma correcao geral for possivel.
- Usar config por jogo para comportamento caro ou experimental.
- Retestar Forza/baseline quando tocar GPU, APU, VFS, threading ou timing.
