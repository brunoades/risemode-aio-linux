# Rise Mode Aura Ice no Linux

Módulo HID para o display USB `aa88:8666` e serviço que envia a temperatura da CPU. Compilado e carregado no Fedora 45, kernel `7.2.15-300.fc45.x86_64`. Uma escrita no display retornou sucesso; a resposta visual do hardware ainda precisa ser confirmada.

O dispositivo observado possui uma interface HID, um endpoint interrupt OUT `0x02` e um relatório de saída de 8 bytes. O módulo usa o pacote `[temperatura, 0x1c, 0, 0, 0x1c, 0, 0, 0]` descrito por implementações comunitárias do Rise Aura Ice. A API HID do kernel recebe antes dele um byte zero de Report ID, que o transporte USB remove. Isso controla somente o **display**. Não há evidência de protocolo USB para alterar a rotação da bomba ou ventoinhas.

## Compilar

```sh
make
```

Requer `kernel-devel` correspondente a `uname -r` e ferramentas de compilação. O arquivo produzido é `hid-risemode-aio.ko`.

## Carregar e testar manualmente

O dispositivo estava ligado ao `hid-generic` e passou automaticamente para `hid-risemode-aio` ao carregar o módulo neste PC. Confirme o endereço HID em `/sys/bus/hid/devices/`, pois o sufixo pode mudar após reconectar o USB:

```sh
sudo insmod ./hid-risemode-aio.ko
readlink /sys/bus/hid/devices/0003:AA88:8666.000A/driver
printf '%s\n' 42 | sudo tee /sys/bus/hid/devices/0003:AA88:8666.000A/display_temp
```

Se o módulo não assumir o dispositivo, faça a troca apenas para o endereço HID identificado:

```sh
printf '%s' '0003:AA88:8666.000A' | sudo tee /sys/bus/hid/drivers/hid-generic/unbind
printf '%s' '0003:AA88:8666.000A' | sudo tee /sys/bus/hid/drivers/hid-risemode-aio/bind
```

Para voltar ao driver padrão, pare o serviço de atualização antes de descarregar o módulo:

```sh
sudo systemctl stop rise-aio-monitor.service
printf '%s' '0003:AA88:8666.000A' | sudo tee /sys/bus/hid/drivers/hid-risemode-aio/unbind
printf '%s' '0003:AA88:8666.000A' | sudo tee /sys/bus/hid/drivers/hid-generic/bind
sudo rmmod hid_risemode_aio
```

Para acompanhar a temperatura real após o módulo estar ligado:

```sh
sudo python3 rise_aio_monitor.py --once
sudo python3 rise_aio_monitor.py
```

Para instalar o módulo e o serviço de forma persistente após confirmar que o visor responde:

```sh
sudo bash install.sh
systemctl status rise-aio-monitor.service
```

O instalador copia o `.ko` para `/lib/modules/$(uname -r)/extra`, registra o carregamento em `/etc/modules-load.d/hid-risemode-aio.conf`, copia o monitor para `/usr/local/libexec/` e habilita o serviço. Mudanças de kernel exigem recompilação e nova instalação para a nova versão.

Para desativar a instalação:

```sh
sudo systemctl disable --now rise-aio-monitor.service
sudo rm /etc/systemd/system/rise-aio-monitor.service /etc/modules-load.d/hid-risemode-aio.conf /usr/local/libexec/rise-aio-monitor.py
sudo rm /lib/modules/$(uname -r)/extra/hid-risemode-aio.ko
sudo depmod -a
sudo rmmod hid_risemode_aio
```

## Referências do protocolo

- [Implementação comunitária de 8 bytes para Rise Aura Ice](https://github.com/bmortella/wc-aura-ice-linux/blob/main/main.py)
- [Outra implementação do mesmo ID USB](https://github.com/mombachm/rise-aio-display-linux/blob/main/rise_aio_usb_settemp.py)
- [Documentação do subsistema HID do kernel](https://www.kernel.org/doc/html/latest/hid/hid-transport.html)
