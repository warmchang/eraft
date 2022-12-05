[![License](https://img.shields.io/badge/license-MIT-green)](https://opensource.org/licenses/MIT)

中文 | [English](README_en.md)

## 运行 raft kv 服务复制组

```
./target/debug/eraftkv-ctl 0 '[::1]:8088'
./target/debug/eraftkv-ctl 1 '[::1]:8089'
./target/debug/eraftkv-ctl 2 '[::1]:8090'
```

## 支持的 SQL 

### CREATE DATABASE

```
./target/debug/eraftproxy-ctl 'http://[::1]:8088' "CREATE DATABASE testdb_1;"
```

### SHOW DATABASES
```
./target/debug/eraftproxy-ctl 'http://[::1]:8088' "SHOW DATABASES;"
```

### CREATE TABLE

```

./target/debug/eraftproxy-ctl 'http://[::1]:8088' "CREATE TABLE classtab ( Name VARCHAR(100), Class VARCHAR(100), Score INT, PRIMARY KEY(Name));"
```

### SHOW CREATE TABLE

```
./target/debug/eraftproxy-ctl 'http://[::1]:8088' "SHOW CREATE TABLE classtab;"
```

### INSERT INTO

```
./target/debug/eraftproxy-ctl 'http://[::1]:8088' "INSERT INTO classtab (Name,Class,Score) VALUES ('Tom', 'B', '88');";
```

### SELECT BY PRIMARY KEY

```
./target/debug/eraftproxy-ctl 'http://[::1]:8088' "SELECT * FROM classtab WHERE Name = 'Tom';"
```




