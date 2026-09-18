# nginx

A more realistic example: take a snapshot of a native nginx process
immediately before it calls `ngx_http_parse_request_line`, restore that state
inside Qiling, and inspect the HTTP request that is about to be parsed.

> **Recommended:** run this example inside the Docker environment provided by
> the `Dockerfile` in the project root. It provides a reproducible Ubuntu 24.04
> environment with the required build tools and dependencies.

## 1. Start the development container

From the project root:

```bash
docker build -t process-bridge .
```

Then start the container with the project mounted into `/workspace`:

```bash
docker run --rm -it \
    -v "$PWD:/workspace" \
    process-bridge
```

Continue with the remaining steps inside the container.

## 2. Get a matching nginx build

`setup_nginx.sh` installs the exact nginx build this example was set up
against. The version is pinned so that the instruction address used for the
breakpoint remains valid:

```bash
sudo bash setup_nginx.sh
# apt-get install -y nginx=1.24.0-2ubuntu7 nginx-common=1.24.0-2ubuntu7
```

The example expects the resulting binary at:

```text
/usr/sbin/nginx
```

## 3. Run nginx with this example's configuration

`nginx.conf` runs a single nginx process in the foreground, with logging sent
to `stderr`:

```nginx
master_process off;
daemon off;
worker_processes  1;
```

This avoids nginx's usual master/worker process management and daemonization,
so `process-bridge` can trace the process directly.

The configuration listens on port 80 and serves static files. A request is
enough to make nginx reach `ngx_http_parse_request_line`.

## 4. Find the `call ngx_http_parse_request_line` instruction

The breakpoint must be placed on the **`call` instruction that invokes**
`ngx_http_parse_request_line`, not on the first instruction of
`ngx_http_parse_request_line` itself.

This is important because the snapshot is taken immediately before the call is
executed. At that point, the function arguments are still in the registers
set up by the caller.

Find the call instruction with `objdump`:

```bash
objdump -D /usr/sbin/nginx | grep 'call.*ngx_http_parse_request_line'
```

For example:

```text
401234: e8 56 78 9a ff    call   39abc0 <ngx_http_parse_request_line>
```

In this example, the breakpoint offset is:

```text
0x401234
```

Use the address of the `call` instruction itself.

The exact address depends on the nginx build, which is why this example uses a
pinned nginx version.

## 5. Take the snapshot

Install `process-bridge` from the project root:

```bash
pip install .
```

Before starting `process-bridge`, schedule a delayed HTTP request:

```bash
(sleep 3; curl -v http://127.0.0.1/) &
```

The delayed request is necessary because `process-bridge` waits for nginx to
reach the breakpoint. The request must therefore be started independently so
that it arrives after nginx has been launched.

Now start nginx through the x86_64 native component:

```bash
process-bridge-x86_64-linux \
    0x401234 \
    /usr/sbin/nginx \
    -c /path/to/examples/nginx/nginx.conf
```

Replace `401234` with the address of the `call
ngx_http_parse_request_line` instruction found in the previous step.

When the delayed `curl` request reaches nginx, execution stops at the `call`
instruction and `process-bridge` creates the snapshot in:

```text
ql_snapshot
```

To enable detailed diagnostic output:

```bash
(sleep 3; curl -v http://127.0.0.1/) &

PROCESS_BRIDGE_LOG_LEVEL=DEBUG \
process-bridge-x86_64-linux \
    0x401234 \
    /usr/sbin/nginx \
    -c /path/to/examples/nginx/nginx.conf
```

## 6. Resume the snapshot in Qiling

`emu_script.py` loads the snapshot, hooks the restored instruction pointer,
and reads the `ngx_buf_t` passed as the second argument to
`ngx_http_parse_request_line`:

```python
from process_bridge import snaphot_init


def read_data(ql):
    b_ptr = ql.arch.regs.rsi

    pos = ql.mem.read_ptr(b_ptr)
    last = ql.mem.read_ptr(b_ptr + 0x08)

    data = ql.mem.read(pos, last - pos)
    print(data)


if __name__ == "__main__":
    ql, entry = snaphot_init.from_snapshot(
        "x86_64",
        "dummy_rootfs",
        "ql_snapshot",
    )

    read_data(ql)

    ql.emu_start(
        begin=entry,
        end=0,
    )
```

`entry` is the restored value of `rip`, which points to the
`call ngx_http_parse_request_line` instruction where the snapshot was taken.

Under the x86-64 System V calling convention, the second function argument is
passed in `rsi`, so `b_ptr` contains the `ngx_buf_t *b` argument.

The `ngx_buf_t` structure starts with the `pos` and `last` pointers.
`read_data` reads these pointers and prints the bytes between them. At this
point, these bytes contain the HTTP request nginx is about to parse.

The `hook_address` callback runs before the instruction at `entry` is
executed. Therefore, the callback sees the same register and memory state that
was captured by `process-bridge`, immediately before the original `call`.

The required Qiling `rootfs` does not need to contain anything for this
example. `dummy_rootfs` only needs to exist as a directory because Qiling
requires a rootfs during initialization. The default mappings created by
Qiling are replaced with the mappings restored from the snapshot.

Create the directory and run the script:

```bash
mkdir dummy_rootfs
python3 emu_script.py
```

## 7. What this demonstrates

At this point we have a snapshot of a **real native nginx process** at a
specific execution point, including the actual request buffer that nginx is
about to process.

The snapshot can now be restored repeatedly, allowing the request data to be
modified between runs. This makes it possible to study how nginx processes
different inputs without having to drive the application from the beginning
each time, and provides a convenient basis for fuzzing the code that follows.

Reaching such a state directly in an emulator can be difficult for a complex
application like nginx, since reproducing its entire execution path and
surrounding system state in the emulated environment may require substantial
setup. `process-bridge` solves this by reaching the desired state natively and
capturing it, then handing that state over to Qiling for further execution,
instrumentation, and repeated experimentation.

