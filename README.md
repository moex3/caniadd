# Caniadd will add files to an AniDB list.

None of the already existing clients did exactly what I wanted, so here I am.
Caniadd is still in developement, but is is already usable.
That is, it implements logging in, encryption, ed2k hashing and adding files, basic ratelimit, keepalive, and file caching.

In the future I want to write an mpv plugin that will use the cached database from caniadd to automatically mark an episode watched on AniDB.
That will be the peak *comfy* animu list management experience.

# Install

You'll need to download the developement package for sqlite3.
Caniadd also needs pthreads, but that's probably already there.

```bash
git clone --recurse-submodules https://git.lain.church/x3/caniadd
cd caniadd
make
./caniadd -h
```

## Things to do:
- NAT handling
- Multi thread hashing
- Read/write timeout in net
- Decode escaping from server
- Use a config file
- Add newline escape to server
- Add myliststats cmd as --stats arg
- Add support for compression
- Implement session saving between different invocations, and session destroying
- Automatically remove found anime from wishlist
- Make deleting from mylist possible, with
  - Name regexes,
  - If file is not found at a scan
- Buffer up mylistadd api cmds when waiting for ratelimit
- Handle C-c gracefully at any time
- Rework cmd line args
  - Should be multiple 'menus' like `caniadd add [paths...]`, `caniadd uptime`, `caniadd watched`...
- After some time passes between now and cache moddate and it's not watched, then query and update it
- Update cache entry is modify is used
- Pretty hashing with color and progress bars and the other fancy stuff
- Keep track of files not in AniDB. Maybe with a NULL lid?
- Write -h page, and maybe a man page too
