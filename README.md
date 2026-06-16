```
    |\__/,|   (`\
  _.|o o  |_   ) )
-(((---(((--------

artist: unkown  [ASCII art archive]
```
    
## Security & Input Access

Naughty Cat reads raw keyboard events from `/dev/input/event4` to detect
keypresses. This requires elevated access to the input subsystem, which is
handled as transparently and minimally as possible.

### How it works

A dedicated system user (`naughty_cat_user`) runs the input daemon. This user:
- belongs only to `naughty_cat_group`, which has read access to `event4`
- has no login shell (`/sbin/nologin`)
- cannot be logged into interactively
- owns the daemon binary exclusively

Your interactive account is not added to the `input` group or any
input-related group. It is added only to `naughty_cat_daemon_group`, which
controls access to the IPC socket — not to any input device.

### What the daemon reads

The daemon receives raw `input_event` structs from the kernel, which include
which key was pressed (`ev.code`), the event type, and a timestamp. The daemon
immediately discards all of this except whether a key was pressed or released
(`ev.value`). This simplified signal is what gets sent to the renderer.

The daemon never logs, stores, or forwards which key was pressed.

### Why not just add the user to the input group?

Adding your user to the `input` group grants every process you run access to
all input devices for the lifetime of your session. The daemon user approach
scopes access to a single process, a single device, with no interactive
footprint.

### Socket permissions

The IPC socket at `/tmp/naughty_cat.sock` is restricted to `naughty_cat_user`
and members of `naughty_cat_daemon_group`. Your interactive account is added to
this group so the renderer can connect, but this group has no access to input
devices — only to the socket file itself.

### Setup

**1. Create the groups and user**

```bash
sudo groupadd naughty_cat_group
sudo groupadd naughty_cat_daemon_group
sudo useradd -r -s /sbin/nologin -M naughty_cat_user
sudo usermod -aG naughty_cat_group naughty_cat_user
sudo usermod -aG naughty_cat_daemon_group naughty_cat_user
sudo usermod -aG naughty_cat_daemon_group $USER
```

**2. Create the udev rule**

Create `/etc/udev/rules.d/99-naughty_cat.rules` with:
KERNEL=="event4", GROUP="naughty_cat_group", MODE="0640"

Then reload:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

**3. Set binary ownership**

```bash
sudo chown naughty_cat_user:naughty_cat_group ./daemon
sudo chmod 750 ./daemon
```

**4. Remove yourself from the input group if present**

```bash
sudo gpasswd -d $USER input
```

**5. Log out and back in**

Group changes don't take effect until you do.

**6. Verify**

```bash
groups                            # should show naughty_cat_daemon_group, NOT input or naughty_cat_group
getent group naughty_cat_group    # should show naughty_cat_user only
getent group naughty_cat_daemon_group  # should show naughty_cat_user and your user
ls -la /dev/input/event4          # group = naughty_cat_group, permissions 640
ls -la ./daemon                   # naughty_cat_user:naughty_cat_group, permissions 750
ls -la /tmp/naughty_cat.sock      # group = naughty_cat_daemon_group, permissions 660
getent passwd naughty_cat_user    # should show /sbin/nologin
```

**7. Run**

```bash
# terminal 1 — start daemon first
sudo -u naughty_cat_user ./daemon

# terminal 2 — start renderer
./anim
```

You can audit the full setup in `install.sh` and the daemon source in
`bongo_daemon.c`.