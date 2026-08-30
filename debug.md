# Debug E Compatibilidade UWP

Este arquivo organiza o fluxo de investigação de jogos no fork UWP/Xbox. A ideia
é coletar evidência primeiro, pesquisar relatos existentes, testar flags de forma
isolada e só depois alterar código.

## Checklist Obrigatório Por Jogo

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
- Se o jogo apareceu em `recent.toml`.
- Se o caminho em `recent.toml` bate com o selecionado.
- Se o `xenia.log` foi gerado.
- Busca externa feita antes de abrir nova hipótese.
- Links de issues, posts, wikis ou relatos relacionados encontrados.

## Busca Externa Obrigatória

Antes de tratar um bug como novo ou mexer em código, fazer uma busca na internet
para verificar se o mesmo problema já foi relatado por alguém.

Pesquisar pelo nome do jogo, Title ID, sintoma exato, tela onde falha, mensagem
do log e termos como `Xenia`, `Xenia Canary`, `UWP`, `Xbox`, `crash`, `freeze`,
`black screen`, `missing text`, `audio continues`, `loading stuck` e nomes de
APIs que aparecem no log.

Registrar no debug do jogo:
- Links encontrados.
- Se o relato é de Xenia desktop, Canary, UWP ou Xbox.
- Configs sugeridas no relato.
- Se o sintoma bate exatamente ou só parece parecido.
- Se existe issue aberta, fix upstream, workaround conhecido ou regressão.

Não copiar hacks específicos de jogo sem validar se existe uma correção geral.

## Fluxo Esperado De Abertura

1. App UWP inicia.
2. Config é carregada.
3. Storage e content roots são resolvidos.
4. Jogo aparece em `recent.toml`.
5. Arquivo/pasta do jogo é identificado.
6. VFS monta o conteúdo.
7. XEX principal é carregado.
8. Imports XEX/XDL são resolvidos.
9. Módulos importados são carregados.
10. `DLL_PROCESS_ATTACH` é executado.
11. Threads guest são criadas.
12. Shader storage inicializa.
13. Jogo entra no menu.
14. Perfil/save/content são acessados.
15. Gameplay começa.

## Ordem Recomendada De Investigação

1. Começar da versão atual `1.3.0`.
2. Instalar bundle estável no Xbox.
3. Rodar o jogo uma vez.
4. Copiar `xenia.log`, config e `recent.toml`.
5. Confirmar Title ID e caminho.
6. Classificar onde falha.
7. Fazer busca externa obrigatória pelo jogo, Title ID e sintoma.
8. Registrar links, configs e workarounds encontrados.
9. Se falha antes do menu, olhar VFS/loader/imports/storage.
10. Se falha no menu, olhar XAM/perfil/save/content.
11. Se falha no loading, olhar VFS/XContent/APU/waits/GPU fence.
12. Se falha no gameplay, olhar GPU/APU/threading/waits/VFS.
13. Se FPS baixo, testar resolução/config antes de código.
14. Se for problema visual, testar `d3d12_readback_resolve` isoladamente.
15. Fazer uma mudança mínima.
16. Rodar format.
17. Rodar lint.
18. Buildar UWP incremental.
19. Testar no Xbox.
20. Comparar com Forza se tocar subsistema sensível.

## Diagnóstico Por Fase Ou Sintoma

### Falha Antes De Abrir O Jogo

Investigar:
- Caminho inválido.
- Permissão de acesso ao armazenamento externo.
- `broadFileSystemAccess`.
- `removableStorage`.
- Formato do jogo.
- Detecção de assinatura.
- Montagem VFS.
- Config de storage.
- Se `recent.toml` confirma o mesmo caminho que foi selecionado.

### Falha Logo Depois De Selecionar O Jogo

Investigar:
- Loader XEX.
- Imports ausentes.
- Módulos XDL/importados.
- Ordem de inicialização de módulos.
- Stubs de kernel usados no boot.
- Falha de montagem de conteúdo.
- Se há crash depois de thread guest iniciar.

Padrões úteis no log:
- `CompleteLaunch`
- `RunTitle`
- `LaunchPath`
- `imports`
- `unimplemented`
- `stub`
- `user module`
- `XEX`
- `XDL`
- `DllMain`
- `DLL_PROCESS_ATTACH`
- `DLL_THREAD_ATTACH`
- `PROCESS_ATTACH`
- `THREAD_ATTACH`

### Falha No Menu

Investigar:
- Perfil de usuário.
- Login/signin.
- Save.
- Content device.
- Device selector.
- XAM UI.
- XAM content.
- Se o jogo congela quando não existe perfil dentro do jogo.
- Se criar perfil/save pelo próprio jogo muda o comportamento.

APIs importantes:
- `XamShowSigninUI`
- `XamUserLogon`
- `XamUserLogonEx`
- `XamUserGetSigninState`
- `XamUserGetSigninInfo`
- `XamUserReadProfileSettings`
- `XamUserWriteProfileSettings`
- `XamShowDeviceSelectorUI`
- `XamContentCreate`
- `XamContentOpenFile`
- `XamContentClose`

### Falha Em Loading

Investigar:
- XContent/SVOD streaming.
- Leituras de arquivos grandes.
- Saves.
- Cache.
- XMA/APU.
- Thread waits.
- GPU fence.
- Shader compilation.

Padrões úteis no log:
- `XContent`
- `ReadSync`
- `SVOD`
- `STFS`
- `XMA`
- `KeWaitForSingleObject`
- `NtWaitForSingleObjectEx`
- `fence`
- `Present`
- `IssueSwap`

### Falha No Gameplay

Se imagem congela e áudio continua:
- Investigar GPU.
- Investigar Present.
- Investigar fence.
- Investigar swap/swapchain.
- Investigar shader/cache.
- Investigar `IssueSwap`.
- Investigar `RefreshGuestOutput`.
- Investigar se o jogo continua gerando frames.

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

Padrões úteis no log:
- `Present`
- `IssueSwap`
- `RefreshGuestOutput`
- `fence`
- `D3D12`
- `GPU`

### FPS Baixo

Primeiro testar sem diagnóstico pesado. Logs podem reduzir FPS e causar stutter
no Xbox/UWP, principalmente quando forçam escrita imediata em disco ou coletam
estatísticas de GPU com alta frequência.

Usar tambem `docs/xbox_360_optimization_notes.md` como mapa de investigacao para
separar gargalos de GPU, CPU/JIT, APU/XMA, VFS e XAM.

Configuração útil para isolar desempenho:

```toml
[Display]
internal_display_resolution = 8

[GPU]
framerate_limit = 60
vsync = true

[D3D12]
d3d12_readback_resolve = false
d3d12_readback_memexport = false
d3d12_submit_on_primary_buffer_end = true
```

Para teste de performance, reduzir resolução interna:
- `internal_display_resolution = 0` usa 640x480.
- `internal_display_resolution = 8` usa 1280x720.

### Problemas Visuais

Para brilho, render-to-texture, texto invisível, menus invisíveis ou efeitos
faltando, testar isoladamente:

```toml
[D3D12]
d3d12_readback_resolve = true
```

Essa opção pode corrigir sintomas visuais em alguns jogos, mas tem impacto forte
em performance. Usar como teste isolado, não como primeira opção para FPS baixo.
Problemas de render-to-texture, tiles, MRT, EDRAM e memexport devem ser
registrados com sintoma, jogo, Title ID, config usada e custo de FPS.

## Logging Para Diagnóstico

Para coletar log de falhas silenciosas, usar temporariamente:

```toml
[Logging]
log_level = 2
flush_log = true
```

Notas:
- `log_level = 2` equivale a Info.
- `flush_log = true` ajuda quando o jogo congela antes do logger descarregar o buffer.
- Depois do diagnóstico, voltar para configuração mais leve se o desempenho for afetado.

Para jogar ou validar performance, manter desligado:

```toml
[Logging]
flush_log = false
log_fps = false

[GPU]
log_draw_stats = false
log_viz_query_stats = false
```

Impacto esperado:
- `log_fps = true`: baixo, mas ainda escreve periodicamente no log.
- `flush_log = true`: médio/alto em UWP, pois força descarregamento imediato do buffer para armazenamento.
- `log_draw_stats = true`: médio/alto em cenas pesadas, pois coleta e grava estatísticas de draws/primitive.
- `log_viz_query_stats = true`: baixo/médio, útil apenas para diagnóstico de VIZ/occlusion.

Se `xenia.log` existe com 0 bytes:
- Confirmar `log_level`.
- Confirmar `flush_log`.
- Confirmar se o jogo apareceu em `recent.toml`.
- Confirmar se o app realmente iniciou o título.
- Repetir com logging em Info.
- Repetir com `flush_log = true`.
- Considerar congelamento sem erro fatal registrado.

Configs por jogo copiadas para `LocalState/config/<TITLEID>.config.toml` não são
sobrescritas automaticamente. Depois de otimizar ou diagnosticar um jogo, remova
a config local de teste ou desligue manualmente flags pesadas para evitar impacto
permanente.

## Config UWP Recomendada Para Base

Config base observada:

```toml
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
```

## Banco De Opções Do Settings UWP

Use esta seção como referência rápida quando uma opção aparecer na UI ou em uma
config por jogo. Preferir testar uma opção por vez, registrar o sintoma antes e
depois, e voltar ao padrão se não houver melhora clara.

### D3D12

`d3d12_allow_variable_refresh_rate_and_tearing`
- Permite VRR e tearing em fullscreen quando a tela suporta.
- Em telas sem VRR, pode causar tearing em alguns casos.
- Usar para testar latência/apresentação, não como correção geral de crash.

`d3d12_readback_resolve`
- Lê no CPU resultados de render-to-texture/resolve.
- Pode ser necessário em alguns jogos, por exemplo screenshots em saves, menus/textos invisíveis ou efeitos que dependem de textura resolvida.
- Causa sincronização no meio do frame e tem impacto de performance muito alto.
- Usar como teste isolado para problema visual; evitar para FPS baixo.

`d3d12_readback_memexport`
- Lê no CPU dados escritos por memory export em shaders.
- Pode ser necessário em alguns jogos, mas muitos acessam esses dados só na GPU.
- Causa sincronização no meio do frame e tem impacto de performance muito alto.
- Só testar quando houver indício de dependência de memexport no log/sintoma.

`d3d12_submit_on_primary_buffer_end`
- Envia a command list quando um PM4 primary buffer termina, se for possível enviar imediatamente.
- Objetivo principal é reduzir latência de frame.
- Normalmente manter ligado na base UWP.

### Display

`postprocess_antialiasing`
- Anti-aliasing de pós-processamento aplicado à imagem final do jogo.
- Valores: vazio/none, `fxaa`, `fxaa_extreme`.
- `none` ou valor desconhecido não altera a imagem original.
- `fxaa` usa NVIDIA Fast Approximate Anti-Aliasing 3.11 em qualidade normal.
- `fxaa_extreme` usa FXAA 3.11 em qualidade extrema.
- É recomendado quando CAS ou FSR está ativo.

`postprocess_scaling_and_sharpening`
- Efeito de pós-processamento para reamostragem e/ou nitidez da saída final.
- Valores: `bilinear`, `cas`, `fsr`.
- `bilinear` ou valor desconhecido mantém 1:1 quando possível e usa stretching bilinear simples quando precisa reamostrar.
- `cas` usa AMD FidelityFX Contrast Adaptive Sharpening para nitidez em escalas até 2x2 e stretching bilinear adicional acima disso.
- `fsr` usa AMD FidelityFX Super Resolution 1.0 para upscaling de maior qualidade, ou CAS quando não está fazendo upscale ou está fazendo downsample.
- Em escala maior que 2x2, múltiplos passes FSR são usados.

`postprocess_ffx_cas_additional_sharpness`
- Nitidez adicional do AMD FidelityFX CAS, de 0 a 1.
- Valor maior deixa a imagem mais nítida.
- Excesso pode criar artefatos/serrilhado em UI ou bordas.

`postprocess_ffx_fsr_max_upsampling_passes`
- Número máximo de passes de upsampling AMD FidelityFX FSR antes de cair para stretching bilinear após o último passe.
- Cada passe escala até 2x2 o tamanho anterior.
- Exemplo: 1280x720 com 1 passe vai até 2560x1440; com 2 passes vai até 5120x2880, incluindo 3840x2160.
- Não tem efeito se a resolução de saída não for alta o bastante.
- Pode ser reduzido em 4K/8K se múltiplos passes custarem performance demais ou se bordas mais suaves forem desejadas.
- O padrão é o máximo suportado internamente pelo Xenia.

`postprocess_ffx_fsr_sharpness_reduction`
- Redução de nitidez do AMD FidelityFX FSR em stops.
- Valor menor deixa a imagem mais nítida.

`postprocess_dither`
- Aplica dithering da precisão interna para 8 bits por canal para suavizar gradientes.
- Em display 10bpc, os 2 bits inferiores ainda são mantidos, mas ruído é adicionado a eles.
- Desligar pode ser recomendado em 10bpc, dependendo da capacidade real da tela.

`internal_display_resolution`
- Controla a resolução interna base usada pelo jogo/emulador.
- `0` usa 640x480.
- `8` usa 1280x720.
- Reduzir é útil para isolar gargalo de performance.

### GPU

`clear_memory_page_state`
- Atualiza o estado das páginas de memória para expor dados escritos pela GPU.
- Pode corrigir modelos ausentes ou dados de GPU não visíveis para o CPU.
- Pode afetar timing/performance; testar por jogo e comparar com baseline.

`gpu_allow_invalid_fetch_constants`
- Permite constantes inválidas de fetch de textura e vértice.
- É geralmente inseguro porque a constante pode conter valores completamente inválidos.
- Pode contornar erros de tipo de fetch constant em certos jogos até descobrir a causa real.
- Se corrigir boot/render, ainda investigar por que a constante veio inválida.

`dxbc_switch`
- Usa `switch` em vez de `if` para fluxo de controle em shaders DXBC.
- Ligar ou desligar pode melhorar estabilidade dependendo do driver.
- Em AMD, historicamente pode ajudar títulos que crasham com `if` porque o compilador talvez tente achatar o fluxo.
- Em Intel HD Graphics, o caminho com `switch` pode ser ignorado por crash com a instrução.

`native_2x_msaa`
- Usa MSAA 2x nativo do host quando disponível.
- Pode ser desligado para teste de escalabilidade em APIs onde 2x não é obrigatório.
- Desligado, usa 2 amostras de 4x MSAA, com qualidade similar ou pior e maior uso de memória.

`vsync`
- Ativa VSYNC.
- Útil para validar pacing/apresentação.
- Pode mascarar gargalo real em debug de performance.

`framerate_limit`
- Limite máximo de FPS.
- `0` significa ilimitado.
- Padrão 60.
- Com `0` e VSYNC ligado, o VSYNC ainda limita a apresentação.

`draw_resolution_scale_x` e `draw_resolution_scale_y`
- Escala inteira da resolução de renderização, opaca para o jogo.
- Valores 1, 2 e 3 podem ser suportados.
- Acima de 1 depende do dispositivo, incluindo sparse binding/tiled resources, bits de endereço virtual por recurso e outros fatores.
- Efeitos e partes do pipeline podem quebrar porque pixels ficam ambíguos para o jogo e porque half-pixel offset pode virar full-pixel.
- Para debug UWP, testar 1x primeiro em problema visual/performance.

`render_target_path_d3d12`
- Caminho de emulação de render target no Direct3D 12.
- Valores: vazio/any, `rtv`, `rov`.
- `rtv` usa render targets do host e blending/depth/stencil fixos, copiando entre render targets quando necessário.
- `rtv` tem menor precisão por suporte limitado de formatos de pixel, mas normalmente maior performance.
- `rtv` é limitado principalmente por mudanças de layout que exigem cópias.
- `rov` faz empacotamento manual de pixel, blending e depth/stencil em software, com mudanças livres de layout de render target.
- `rov` exige GPU com rasterizer-ordered views.
- `rov` tem maior precisão, mas é limitado principalmente por overdraw.
- Em drivers AMD, `rov` atualmente pode causar crashes no compilador de shader em muitos casos.
- `any` escolhe o que é considerado melhor para o sistema; atualmente tende a RTV porque ROV é muito mais lento, exceto cenários de GPU Intel com bug de stencil.

### Memory

`ignore_offset_for_ranged_allocations`
- Ignora o offset de 4 KB em alocações físicas com range informado.
- Alguns títulos verificam se o resultado bate com o limite inferior informado.
- Testar quando houver crash/comportamento estranho em alocação física ou checks de endereço retornado.

`protect_on_release`
- Protege memória liberada para impedir acessos posteriores.
- Útil para detectar use-after-free ou acessos inválidos.
- Pode transformar bug silencioso em crash mais claro.
- Não é opção de performance.

`protect_zero`
- Protege a página zero contra leituras e escritas.
- Ajuda a capturar null dereference/acesso inválido cedo.
- Manter ligado salvo teste específico.

`scribble_heap`
- Preenche com `0xCD` toda memória heap alocada.
- Útil para revelar dependência de memória não inicializada.
- Pode mudar comportamento de bugs e deve ser usado como diagnóstico, não como workaround permanente.

### Storage

`mount_cache`
- Ativa montagem de cache.
- Muitos jogos esperam cache disponível.
- Manter ligado na base.

`mount_scratch`
- Ativa montagem scratch.
- Normalmente desligado.
- Testar apenas quando o jogo claramente depender desse device ou houver falha relacionada a storage temporário.

`cache_root`, `content_root` e `storage_root`
- Vazios usam os locais padrão do app/UWP.
- Se apontarem para pasta externa, confirmar permissão de escrita e acesso USB.
- Caminho inválido pode causar falha antes do menu, saves ausentes ou cache sem persistência.

### UI

`show_achievement_notification`
- Mostra notificação de conquista na tela.
- Afeta UI/overlay, não compatibilidade de jogo em geral.

### x64

`enable_host_guest_stack_synchronization`
- Registra mapeamentos de stack host/guest no início de funções e checa reentry em retornos.
- Tem pequeno impacto de performance.
- Corrige crashes em jogos que usam `setjmp`/`longjmp`.

### XConfig

`user_language` e `user_country`
- Controlam idioma e país reportados ao jogo por `XGetLanguage`, `ExGetXConfigSetting(XCONFIG_USER_LANGUAGE)` e `XamGetLocale`.
- UI UWP usa English = `user_language 1`, `user_country 103`.
- UI UWP usa Português Brasil = `user_language 9`, `user_country 13`.
- UI UWP usa Español España = `user_language 5`, `user_country 31`.
- Jogos já em execução podem precisar reiniciar para detectar novo idioma.

## Subsistemas Específicos

### Conteúdo E Saves

Para jogos que dependem de perfil/save:
- Testar com perfil criado dentro do jogo.
- Testar sem perfil apenas para reproduzir o problema.
- Confirmar se o jogo cria conteúdo em `content_root`.
- Confirmar se `mount_cache = true`.
- Confirmar se `storage_root` e `content_root` estão vazios ou apontando para locais válidos.
- Confirmar se o jogo usa device selector.
- Confirmar se o jogo usa `XamContentCreate` ou `XamContentOpenFile`.

### APU E XMA

Base aprovada para UWP:

```toml
[APU]
use_dedicated_xma_thread = false
use_new_decoder = false
```

Manter XMA dedicado desligado no UWP.

Quando um jogo trava ao entrar no gameplay ou em loading com áudio:
- Verificar logs XMA.
- Verificar se há frames inválidos.
- Verificar se há offsets inválidos.
- Comparar comportamento com Forza antes de aceitar alteração.

### XContent E SVOD

Para jogos GOD/STFS/SVOD:
- Verificar se o conteúdo monta.
- Verificar se arquivos grandes são lidos.
- Verificar se a leitura trava em loading.
- Confirmar se o caminho do jogo está correto.
- Confirmar se o conteúdo não está corrompido.

O acesso XContent/SVOD deve manter proteção de leitura por arquivo compartilhado.

### Loader XEX/XDL

Para jogo que fecha ou trava logo após launch:
- Verificar módulos importados.
- Verificar imports de usuário.
- Verificar se `DLL_PROCESS_ATTACH` acontece antes de attaches de thread.
- Verificar se algum import aparece como `unimplemented`.
- Verificar se há crash depois de thread guest iniciar.

### Threading E Waits

Para congelamento sem crash:
- Procurar loops repetidos em waits.
- Procurar `NtWaitForSingleObjectEx`.
- Procurar `KeWaitForSingleObject`.
- Procurar waits retornando imediatamente em massa.
- Procurar handle repetido.
- Procurar overlapped pendente.
- Procurar evento que nunca é sinalizado.
- Procurar evento sempre sinalizado causando loop quente.

Exemplo de pista:

```text
NtWaitForSingleObjectEx infinite begin
NtWaitForSingleObjectEx infinite end
duration_ms=0
```

Se isso aparecer milhares de vezes, investigar o tipo do handle e o objeto
esperado.

### GPU E Present

Para tela congelada com áudio vivo:
- Investigar `Present`.
- Investigar swapchain.
- Investigar `IssueSwap`.
- Investigar `RefreshGuestOutput`.
- Investigar fence.
- Investigar shader cache.
- Investigar se o jogo continua gerando frames.

### Stubs E Funções Não Implementadas

Se o log mostra função `!!` ou `unimplemented` perto da falha:
- Identificar módulo.
- Identificar função.
- Ver se é chamada antes da falha.
- Ver se retorna erro fixo.
- Ver se precisa completar overlapped.
- Ver se precisa disparar notification.
- Ver se precisa preencher struct de saída.
- Implementar comportamento mínimo geral, sem lógica por jogo.

### Overlapped E Notifications

Muitos jogos esperam operações assíncronas de XAM/kernel.

Verificar se a função:
- Retorna `X_ERROR_IO_PENDING` quando recebe overlapped.
- Chama `CompleteOverlappedImmediate`.
- Chama `CompleteOverlappedDeferredEx`.
- Preenche `extended_error`.
- Preenche `length`.
- Sinaliza o evento correto.
- Dispara notifications esperadas.

Áreas comuns:
- `XamContentCreate`
- `XamContentOpenFile`
- `XamUserReadProfileSettings`
- `XamUserWriteProfileSettings`
- `XamShowDeviceSelectorUI`
- `XamShowSigninUI`

## Subsistemas Sensíveis

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

## Critério Para Aceitar Uma Correção

Uma correção deve:
- Resolver o sintoma reproduzido.
- Ser geral, não específica por Title ID.
- Ter log ou teste que justifique a mudança.
- Ter busca externa registrada quando for bug de jogo/compatibilidade.
- Não piorar Forza.
- Não depender de comportamento frágil do menu.
- Não aumentar logging permanente de forma pesada.
- Passar em format.
- Passar em lint.
- Buildar UWP.
- Ser testada no Xbox real.

## Checklist Rápido Por Jogo

```text
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
Busca externa feita:
Links relacionados:
Primeiro subsistema suspeito:
Próximo teste:
```
