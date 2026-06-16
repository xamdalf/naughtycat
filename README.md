```
    |\__/,|   (`\
  _.|o o  |_   ) )
-(((---(((--------
```
artist: unkown  [ASCII art archive]

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
input-related group.

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



### Setup

**1. Create the group and user**

```bash
sudo groupadd naughty_cat_group
sudo useradd -r -s /sbin/nologin -M naughty_cat_user
sudo usermod -aG naughty_cat_group naughty_cat_user
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
sudo chown naughty_cat_user:naughty_cat_group ./bongo
sudo chmod 750 ./bongo
```

**4. Remove yourself from the input group if present**

```bash
sudo gpasswd -d $USER input
```

Log out and back in for group changes to take effect.

**5. Verify**

```bash
groups                        # should NOT show input or naughty_cat_group
getent group naughty_cat_group  # should show only naughty_cat_user
ls -la /dev/input/event4      # should show group = naughty_cat_group, permissions 640
ls -la ./bongo                # should show naughty_cat_user:naughty_cat_group, permissions 750
getent passwd naughty_cat_user  # should show /sbin/nologin
```

**6. Run the daemon**

```bash
sudo -u naughty_cat_user ./bongo
```

You can audit the full setup in `install.sh` and the daemon source in
`bongo_daemon.c`.
