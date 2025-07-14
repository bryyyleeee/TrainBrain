## Install needed libraries

```bash
sudo apt-get install libmariadb-dev
sudo apt install libmariadb-dev-compat
```

## compile it with


```bash
gcc -o myapp myapp.c $(mysql_config --cflags --libs)
```