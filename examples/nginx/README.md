# nginx

A more realistic example: snapshot a running nginx worker at a breakpoint
inside its own code, then resume execution inside Qiling and inspect a
buffer nginx is working with at that point.

## 1. Get a matching nginx build

`setup_nginx.sh` installs the exact nginx build this example was set up
against (pin the version so the breakpoint offset you find with `nm`/`objdump`
stays valid):

```bash
sudo bash setup_nginx.sh
# apt-get install -y nginx=1.24.0-2ubuntu7 nginx-common=1.24.0-2ubuntu7
```

## 2. Run it with this example's config

`nginx.conf` runs a single worker in the foreground, logging to `stderr`, so
`process-bridge` can trace it directly without dealing with nginx's usual
master/worker forking and daemonization:

```nginx
master_process off;
daemon off;
worker_processes  1;
```

It listens on port 80 and serves static files, which is enough to trigger
`ngx_http_parse_request_line`, the function this example breaks into.

## 3. Find the breakpoint offset

This example breaks at `ngx_http_parse_request_line`:

```c
ngx_int_t
ngx_http_parse_request_line(ngx_http_request_t *r, ngx_buf_t *b)
```

It's a good breakpoint target for this walkthrough because its second
argument (`b`, passed in `rsi` per the x86-64 SysV calling convention) is the
`ngx_buf_t` holding the raw request line nginx is about to parse — exactly
what `read_data` in `emu_script.py` (below) reads out.

Find its address with `nm` or `objdump -d` on the `nginx` binary, and use
that address as the hex offset — nginx's stock builds aren't PIE, so this is
the function's address directly, not an offset from a runtime base:

```bash
nm -D /usr/sbin/nginx | grep ngx_http_parse_request_line
```

## 4. Take the snapshot

```bash
process-bridge-x64 <offset> /usr/sbin/nginx -c /path/to/examples/nginx/nginx.conf
```

Then send it a request (e.g. `curl localhost/`) so it actually reaches the
breakpoint; `process-bridge-x64` waits for that hit before dumping
`ql_snapshot`.

## 5. Resume it in Qiling and read the buffer

`emu_script.py` hooks the restored entry point and, before letting execution
continue, reads out the buffer that `ngx_http_parse_request_line` received as
its `b` argument (`rsi`):

```python
def read_data(ql: Qiling) -> None:
    b_ptr = ql.arch.regs.rsi

    pos = ql.mem.read_ptr(b_ptr)
    last = ql.mem.read_ptr(b_ptr + 0x08)

    data = ql.mem.read(pos, last - pos)
    print(data)


if __name__ == "__main__":
    ql, entry = snaphot_init.from_snapshot(
        "x86_64", "dummy_rootfs", "ql_snapshot", QL_VERBOSE.DEBUG
    )
    try:
        ql.hook_address(callback=read_data, address=entry)
        ql.emu_start(begin=entry, end=0)
    except UcError as e:
        ...
```

`b_ptr` is that `ngx_buf_t *b`; its first two fields are the `pos` and `last`
pointers, so `read_data` dereferences both and prints the bytes in between —
the raw request line nginx is about to parse at the moment the snapshot was
taken. `hook_address` fires the callback right as execution resumes at
`entry`, before any instruction of `ngx_http_parse_request_line` itself
runs, so `rsi` still holds the untouched `b` argument from the original
call. `emu_start(..., end=0)` then just lets Qiling run free from there; the
`except UcError` block (same as in `examples/hello`) dumps the faulting
address and disassembly if the emulator hits unmapped memory or an
unsupported instruction.

Run it the same way as the `hello` example:

```bash
mkdir -p dummy_rootfs
python3 emu_script.py
```
