# Caniadd will add files to an AniDB list.

None of the already existing clients did exactly what I wanted, so here I am.
Caniadd is still in developement, but is is already usable.
That is, it implements logging in, encryption, ed2k hashing and adding files, basic ratelimit, keepalive, and file caching.

In the future I want to write an mpv plugin that will use the cached database from caniadd to automatically mark an episode watched on AniDB.
That will be the peak *comfy* animu list management experience.

## Things to do:
- NAT handling
- Multi thread hashing
- Read/write timeout in net
- Api ratelimit (the other part)
- Decode escaping from server
- Use a config file
- Add newline escape to server
- Better field parsing, remove the horrors at code 310
- Add myliststats cmd as --stats arg
- Add support for compression
- Make deleting from mylist possible, with
  - Name regexes,
  - If file is not found at a scan
- Use api\_cmd style in api\_encrypt\_init
- Buffer up mylistadd api cmds when waiting for ratelimit
- Handle C-c gracefully at any time
- Write -h page, and maybe a man page too
