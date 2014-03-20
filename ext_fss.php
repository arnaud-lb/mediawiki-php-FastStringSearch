<?hh

<<__Native>>
function fss_prep_search( mixed $search ): mixed;

<<__Native>>
function fss_exec_search( resource $r, string $haystack, int $offset = 0 ): mixed;

<<__Native>>
function fss_prep_replace( array $pairs ): resource;

<<__Native>>
function fss_exec_replace( resource $r, string $subject ): mixed;

<<__Native>>
function fss_free( resource $r ): bool;

