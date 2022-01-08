#ifndef _UIO_H
#define _UIO_H

void uio_user(const char *format, ...) __attribute__((format (printf, 1, 2)));
void uio_error(const char *format, ...) __attribute__((format (printf, 1, 2)));
void uio_debug(const char *format, ...) __attribute__((format (printf, 1, 2)));
void uio_warning(const char *format, ...) __attribute__((format (printf, 1, 2)));

#endif /* _UIO_H */
