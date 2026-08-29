Informações Obrigatórias Por Jogo
Registrar sempre:
- Nome do jogo.
- Title ID.
- Caminho completo no Xbox.
- Formato do jogo: GOD, XEX solto, ISO/XISO, STFS ou SVOD.
- Tela exata onde trava.
- Se trava antes do menu, no menu, em loading ou no gameplay.
- Se acontece com perfil criado.
- Se acontece sem perfil criado.
- Se gera save.
- Se o áudio continua.
- Se a imagem congela.
- Se o app fecha ou só para de responder.
- Config usada no Xbox.
- Bundle/teste usado.
Como Interpretar Abertura Do Jogo
Fluxo esperado:
 1. App UWP inicia.
 2. Config é carregada.
 3. Storage e content roots são resolvidos.
 4. Jogo aparece em recent.toml.
 5. Arquivo/pasta do jogo é identificado.
 6. VFS monta o conteúdo.
 7. XEX principal é carregado.
 8. Imports XEX/XDL são resolvidos.
 9. Módulos importados são carregados.
10. DLL_PROCESS_ATTACH é executado.
11. Threads guest são criadas.
12. Shader storage inicializa.
13. Jogo entra no menu.
14. Perfil/save/content são acessados.
15. Gameplay começa.
Falha Antes De Abrir O Jogo
Investigar:
- Caminho inválido.
- Permissão de acesso ao armazenamento externo.
- broadFileSystemAccess.
- removableStorage.
- Formato do jogo.
- Detecção de assinatura.
- Montagem VFS.
- Config de storage.
Confirmar em recent.toml se o caminho é o mesmo que foi selecionado.
Falha Logo Depois De Selecionar O Jogo
Investigar:
- Loader XEX.
- Imports ausentes.
- Módulos XDL/importados.
- Ordem de inicialização de módulos.
- Stubs de kernel usados no boot.
- Falha de montagem de conteúdo.
Padrões úteis no log:
CompleteLaunch
RunTitle
LaunchPath
imports
unimplemented
stub
XEX
XDL
DllMain
DLL_PROCESS_ATTACH
DLL_THREAD_ATTACH
Falha No Menu
Investigar:
- Perfil de usuário.
- Login/signin.
- Save.
- Content device.
- Device selector.
- XAM UI.
- XAM content.
APIs importantes:
XamShowSigninUI
XamUserLogon
XamUserLogonEx
XamUserGetSigninState
XamUserGetSigninInfo
XamUserReadProfileSettings
XamUserWriteProfileSettings
XamShowDeviceSelectorUI
XamContentCreate
XamContentOpenFile
XamContentClose
Se o jogo congela quando não existe perfil dentro do jogo, testar criando o perfil/save pelo próprio jogo e repetir.
Falha Em Loading
Investigar:
- XContent/SVOD streaming.
- Leituras de arquivos grandes.
- Saves.
- Cache.
- XMA/APU.
- Thread waits.
- GPU fence.
- Shader compilation.
Padrões úteis:
XContent
ReadSync
SVOD
STFS
XMA
KeWaitForSingleObject
NtWaitForSingleObjectEx
fence
Present
IssueSwap
Falha No Gameplay
Se imagem congela e áudio continua:
- Investigar GPU.
- Investigar Present.
- Investigar fence.
- Investigar swap.
- Investigar shader/cache.
Se imagem e áudio travam juntos:
- Investigar CPU/threading.
- Investigar waits.
- Investigar APU/XMA.
- Investigar VFS.
Se loading continua indefinidamente:
- Investigar I/O.
- Investigar XContent/SVOD.
- Investigar XAM content.
- Investigar evento/overlapped não completado.
FPS Baixo
Primeiro testar sem diagnóstico pesado.
Configurações úteis para isolar desempenho:
[Display]
internal_display_resolution = 8

[GPU]
framerate_limit = 60
vsync = true

[D3D12]
d3d12_readback_resolve = false
d3d12_readback_memexport = false
d3d12_submit_on_primary_buffer_end = true
Para teste de performance, reduzir resolução interna:
[Display]
internal_display_resolution = 0
internal_display_resolution = 0 usa 640x480.
internal_display_resolution = 8 usa 1280x720.
Problemas Visuais
Para brilho, render-to-texture ou efeitos faltando, testar isoladamente:
[D3D12]
d3d12_readback_resolve = true
Essa opção pode corrigir sintomas visuais em alguns jogos, mas tem impacto forte em performance.
Usar como teste isolado, não como primeira opção para FPS baixo.
Config UWP Recomendada Para Base
Config base observada:
[APU]
apu = "any"
apu_max_queued_frames = 64
use_dedicated_xma_thread = false
use_new_decoder = false

[D3D12]
d3d12_readback_memexport = false
d3d12_readback_resolve = false
d3d12_submit_on_primary_buffer_end = true

[Display]
internal_display_resolution = 8

[GPU]
framerate_limit = 60
vsync = true

[Kernel]
ignore_thread_affinities = true
ignore_thread_priorities = true
max_signed_profiles = 4

[Storage]
cache_root = ""
content_root = ""
mount_cache = true
mount_scratch = false
storage_root = ""
Logging Para Diagnóstico
Para coletar log de falhas silenciosas, usar temporariamente:
[Logging]
log_level = 2
flush_log = true
log_level = 2 equivale a Info.
flush_log = true ajuda quando o jogo congela antes do logger descarregar o buffer.
Depois do diagnóstico, voltar para configuração mais leve se o desempenho for afetado.
Quando O Log Fica Vazio
Se xenia.log existe com 0 bytes:
- Confirmar log_level.
- Confirmar flush_log.
- Confirmar se o jogo apareceu em recent.toml.
- Confirmar se o app realmente iniciou o título.
- Repetir com logging em Info.
- Repetir com flush_log = true.
Log vazio pode indicar congelamento sem erro fatal registrado.
Conteúdo E Saves
Para jogos que dependem de perfil/save:
- Testar com perfil criado dentro do jogo.
- Testar sem perfil apenas para reproduzir o problema.
- Confirmar se o jogo cria conteúdo em content_root.
- Confirmar se mount_cache = true.
- Confirmar se storage_root e content_root estão vazios ou apontando para locais válidos.
- Confirmar se o jogo usa device selector.
- Confirmar se o jogo usa XamContentCreate ou XamContentOpenFile.
APU E XMA
Base aprovada para UWP:
[APU]
use_dedicated_xma_thread = false
use_new_decoder = false
Manter XMA dedicado desligado no UWP.
Quando um jogo trava ao entrar no gameplay ou em loading com áudio:
- Verificar logs XMA.
- Verificar se há frames inválidos.
- Verificar se há offsets inválidos.
- Comparar comportamento com Forza antes de aceitar alteração.
XContent E SVOD
Para jogos GOD/STFS/SVOD:
- Verificar se o conteúdo monta.
- Verificar se arquivos grandes são lidos.
- Verificar se a leitura trava em loading.
- Confirmar se o caminho do jogo está correto.
- Confirmar se o conteúdo não está corrompido.
O acesso XContent/SVOD deve manter proteção de leitura por arquivo compartilhado.
Loader XEX/XDL
Para jogo que fecha ou trava logo após launch:
- Verificar módulos importados.
- Verificar imports de usuário.
- Verificar se DLL_PROCESS_ATTACH acontece antes de attaches de thread.
- Verificar se algum import aparece como unimplemented.
- Verificar se há crash depois de thread guest iniciar.
Padrões úteis:
user module
imports
XEX
XDL
DllMain
PROCESS_ATTACH
THREAD_ATTACH
Threading E Waits
Para congelamento sem crash:
- Procurar loops repetidos em waits.
- Procurar NtWaitForSingleObjectEx.
- Procurar KeWaitForSingleObject.
- Procurar waits retornando imediatamente em massa.
- Procurar handle repetido.
- Procurar overlapped pendente.
- Procurar evento que nunca é sinalizado.
- Procurar evento sempre sinalizado causando loop quente.
Exemplo de pista:
NtWaitForSingleObjectEx infinite begin
NtWaitForSingleObjectEx infinite end
duration_ms=0
Se isso aparecer milhares de vezes, investigar o tipo do handle e o objeto esperado.
GPU E Present
Para tela congelada com áudio vivo:
- Investigar Present.
- Investigar swapchain.
- Investigar IssueSwap.
- Investigar RefreshGuestOutput.
- Investigar fence.
- Investigar shader cache.
- Investigar se o jogo continua gerando frames.
Padrões úteis:
Present
IssueSwap
RefreshGuestOutput
fence
D3D12
GPU
Stubs E Funções Não Implementadas
Se o log mostra função !! ou unimplemented perto da falha:
- Identificar módulo.
- Identificar função.
- Ver se é chamada antes da falha.
- Ver se retorna erro fixo.
- Ver se precisa completar overlapped.
- Ver se precisa disparar notification.
- Ver se precisa preencher struct de saída.
- Implementar comportamento mínimo geral, sem lógica por jogo.
Overlapped E Notifications
Muitos jogos esperam operações assíncronas de XAM/kernel.
Verificar se a função:
- Retorna X_ERROR_IO_PENDING quando recebe overlapped.
- Chama CompleteOverlappedImmediate.
- Chama CompleteOverlappedDeferredEx.
- Preenche extended_error.
- Preenche length.
- Sinaliza o evento correto.
- Dispara notifications esperadas.
Áreas comuns:
XamContentCreate
XamContentOpenFile
XamUserReadProfileSettings
XamUserWriteProfileSettings
XamShowDeviceSelectorUI
XamShowSigninUI
Ordem Recomendada Para Fazer Um Jogo Funcionar
 1. Começar da versão atual 1.2.0.
 2. Instalar bundle estável no Xbox.
 3. Rodar o jogo uma vez.
 4. Copiar xenia.log, config e recent.toml.
 5. Confirmar Title ID e caminho.
 6. Classificar onde falha.
 7. Se falha antes do menu, olhar VFS/loader/imports.
 8. Se falha no menu, olhar XAM/perfil/save.
 9. Se falha no loading, olhar VFS/XContent/APU/waits.
10. Se falha no gameplay, olhar GPU/APU/threading.
11. Se FPS baixo, testar resolução/config antes de código.
12. Se for problema visual, testar d3d12_readback_resolve.
13. Fazer uma mudança mínima.
14. Rodar format.
15. Rodar lint.
16. Buildar UWP incremental.
17. Testar no Xbox.
18. Comparar com Forza se tocar subsistema sensível.
Subsistemas Sensíveis
Retestar com cuidado quando mexer em:
- APU.
- XMA.
- XContent.
- SVOD.
- VFS.
- Filesystem.
- Threading.
- Timing.
- GPU.
- Shader/cache.
- Loader XEX/XDL.
- XAM profile/save.
Forza é o baseline principal para regressão nessas áreas.
Critério Para Aceitar Uma Correção
Uma correção deve:
- Resolver o sintoma reproduzido.
- Ser geral, não específica por Title ID.
- Ter log ou teste que justifique a mudança.
- Não piorar Forza.
- Não depender de comportamento frágil do menu.
- Não aumentar logging permanente de forma pesada.
- Passar em format.
- Passar em lint.
- Buildar UWP.
- Ser testada no Xbox real.
Checklist Rápido Por Jogo
Jogo:
Title ID:
Formato:
Caminho:
Versão do bundle:
Tela onde falha:
Perfil criado:
Save criado:
Áudio continua:
Imagem congela:
App fecha:
xenia.log gerado:
recent.toml confirma jogo:
Config copiada:
Primeiro subsistema suspeito:
Próximo teste:
