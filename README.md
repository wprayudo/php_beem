#PHP_BEEM

Manage PHP Scalability and Bee Instance feature using Bee Messaging Pattern

## Requirements

* PHP 5.3+
* beem 1.0.0

## INSTALL

First , you need to install beem 

```
git clone https://github.com/wprayudo/php_beem.git
phpize
./configure 
make CFLAGS="-I/yourpath/beem/1.0.0/include" LDFLAGS="-L/yourpath/beem/1.0.0/lib -lbeem" 
make install

```

Then add `extension=beem.so` to your `php.ini`.