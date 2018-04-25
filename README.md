# php-ffs for PHP7

> auth: wikimedia organization 
> PHP7_adapter: Vikin

This is a PHP extension for fast string search and replace. It is used by 
StringUtils.php. It supports multiple search terms. We use it as a
replacement for PHP's strtr, which is extremely slow in certain cases.
Chinese script conversion is one of those cases. This extension uses a
Commentz-Walter style algorithm for multiple search terms, or a Boyer-Moore
algorithm for single search terms.

Several source files were taken from GNU grep, and are under the GNU General
Public License. The PHP license is incompatible, so a PHP binary containing this
extension is probably not redistributable under any license. You can use such a
build privately, however. The source files can be distributed under their
respective licenses.

The interface synopsis is as follows. To prepare a string search:

```php
$fss = fss_prep_search( 'Hello' );
//or 
$fss = fss_prep_search( array( 'Hello', 'Hi' ) );
```

To search a string, pass the previously prepared object to fss_exec_search,
along with the subject string (the "haystack"):

```php
$result = fss_exec_search( $fss, 'xxx Hello xxx' );
```

This will return an array with the first element being the string offset of the
match, and the second element being the length of the match. If no matches are
found, false is returned. The first match is returned, and if multiple search
strings match at the same location, the longest will be returned. This function
also accepts an optional third parameter giving the offset at which to start the
search.

Replacements are performed like this:

```php
$fss = fss_prep_replace( array( 'from' => 'to' ) );
$text = fss_exec_replace( $fss, $text );
```

The interpretation of the replacement array is exactly the same as in strtr: the
longest match is always used, and parts of the string which have already been
replaced are not processed again. 

You can free an FSS result with:

```php
fss_free( $fss );
```

This is not generally necessary, since PHP will clean up the memory when all
references are released. 

If you use an FSS object prepared with fss_prep_search() in fss_exec_replace(),
all strings matched will be removed, i.e. replaced by an empty string.

