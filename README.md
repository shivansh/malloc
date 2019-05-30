# malloc

## Usage
```
clang -O0 -g -W -Wall -Wextra -shared -fPIC malloc.c -o malloc.so
LD_PRELOAD=./malloc.so /bin/ls
```

## References
* https://danluu.com/malloc-tutorial/
* http://g.oswego.edu/dl/html/malloc.html
