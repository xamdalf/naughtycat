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

The install script handles:
- creating `naughty_cat_user` and `naughty_cat_group`
- installing a udev rule scoping `/dev/input/event4` to `naughty_cat_group`
- setting binary ownership and permissions

You can audit the full setup in `install.sh` and the daemon source in
`bongo_daemon.c`.
