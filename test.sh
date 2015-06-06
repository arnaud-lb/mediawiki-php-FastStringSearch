#!/usr/bin/env bash
set -e
set -u

DIRNAME="$(dirname $0)"

if [ -r "run-tests.php" -a -r "modules/fss.so" ]; then
  echo "Running PHP5 tests..."
  USE_PHP="${TEST_PHP_EXECUTABLE:-$(which php5)}"
  TEST_PHP_EXECUTABLE="$USE_PHP" REPORT_EXIT_STATUS=1 "$USE_PHP" \
    -n -d open_basedir= -d output_buffering=0 -d memory_limit=-1 \
    run-tests.php -n -d extension_dir=modules -d extension=fss.so tests
  exit $?
elif [ -r "fss.so" -a -x "run-test" ]; then
  echo "Running HHVM tests..."
  ./run-test -a "-vServer.LightProcessCount=0 -vDynamicExtensions.0=fss.so" test
  exit $?
else
  echo "I could find neither a Zend PHP nor HHVM build in this directory."
  exit 1
fi
