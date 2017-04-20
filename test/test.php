<?php

//$fss = fss_prep_search(array('hello', 'hi'));

//var_dump( fss_free($fss) );



//$fss = fss_prep_search(array('hello', 'hi'));
//var_dump(fss_exec_search($fss, 'hhhhello'));
//var_dump(fss_exec_search($fss, 'hhhhello',0));
//var_dump(fss_exec_search($fss, 'hellohello', 1));
//var_dump(fss_exec_search($fss, 'hellohihello', 1));
//var_dump(fss_exec_search($fss, 'adfjshfkjs'));

//var_dump( fss_free($fss) );

//$fss = fss_prep_search('hello');
//var_dump(fss_exec_search($fss, 'hhhhello'));
//var_dump(fss_exec_search($fss, 'hellohello', 1));
//var_dump(fss_exec_search($fss, 'adfjshfkjs'));

//var_dump(fss_exec_replace($fss, 'helloabchelloaa'));

//var_dump( fss_free($fss) );

$fss = fss_prep_replace(array(
        'abc' => 'def',
        'ab' => 'X',
));

$fss = fss_prep_replace( array( "a" => "1", "" => "2", "c" => "3" ) );
var_dump( fss_exec_replace( $fss, "abcde" ) );

//var_dump(fss_exec_replace($fss, 'ddabcababcaaaab'));

