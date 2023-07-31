#!/usr/bin/sh

#
# psoc6/run_psoc6_tests.sh -ci
#


echo "executing $0 $* ..."

usage() {
  echo "Usage:"
  echo "sh run_psoc6_tests.sh <opt>"
  echo "Available options:"
  echo "  -a   run all meaningful test by directory without excluding any"
  echo "  -c  clean results directory and file before running tests"
  echo "  -d  device device to be used"
  echo "  -f  run failing tests only"
  echo "  -i  run implemented tests only and exclude known failing tests"
  echo "  -n  run not yet implemented tests only"
  echo "  -w  run wifi tests => needs secrets.py key file>"
  exit 1;
}


while getopts "acd:fhinw" o; do
  case "${o}" in
    a)
       all=1
       ;;
    c)
       cleanResultsDirectoryFirst=1
       ;;
    d)
       device=${OPTARG}
       ;;
    f)
       failing=1
       ;;
    h)
       usage
       ;;
    i)
       implemented=1
       ;;
    n)
       notYetImplemented=1
       ;;
    w)
       wifi=1
       ;;
    *)
       usage
       ;;
  esac
done


if [ -z "${all}" ]; then
  all=0
fi


if [ -z "${cleanResultsDirectoryFirst}" ]; then
  cleanResultsDirectoryFirst=0
fi


if [ -z "${device}" ]; then
  device="/dev/ttyACM0"
fi


if [ -z "${failing}" ]; then
  failing=0
fi


if [ -z "${implemented}" ]; then
  implemented=0
fi


if [ -z "${notYetImplemented}" ]; then
  notYetImplemented=0
fi


if [ -z "${wifi}" ]; then
  wifi=0
fi


resultsFile="psoc6_test_results.log"
passResultsFile="psoc6_test_passed.log"
skipResultsFile="psoc6_test_skipped.log"
failResultsFile="psoc6_test_failed.log"


echo
echo "  device            : ${device}"
echo
echo "  results file      : ${resultsFile}"
echo "  pass results file : ${passResultsFile}"
echo "  skip results file : ${skipResultsFile}"
echo "  fail results file : ${failResultsFile}"
echo


if [ ${cleanResultsDirectoryFirst} -eq 1 ]; then

  echo
  echo "  removing results directory and file if existent ..."

  if [ -e results ]; then

    rm -rf results ${resultsFile}

  fi

  echo "  done."
  echo

fi


if [ ${all} -eq 1 ]; then

  echo "  running all tests ..."
  echo

  echo "  setting up a network connection ..."
  echo
  
  ../tools/mpremote/mpremote.py run psoc6/test_scripts/network_on.py

  ./run-tests.py --target psoc6 --device ${device} \
        -d \
          basics \
          extmod \
          feature_check \
          float \
          frozen \
          import \
          inlineasm \
          micropython \
          misc \
          multi_bluetooth \
          multi_net \
          net_hosted \
          net_inet \
          perf_bench \
          psoc6 \
          stress \
          thread \
          unicode \
    | tee -a ${resultsFile}

  echo
  echo "  done."
  echo

fi


if [ ${wifi} -eq 1 ]; then

  echo "  setting up a network connection ..."
  echo
  
  ../tools/mpremote/mpremote.py run psoc6/test_scripts/network_on.py


  echo "  running wifi tests ..."
  echo

  ./run-tests.py --target psoc6 --device ${device} \
        -d \
          net_hosted \
          net_inet \
        \
        -e net_inet/ssl_cert.py \
        -e net_inet/test_tls_nonblock.py \
        -e net_inet/tls_text_errors.py \
        \
        -e net_hosted/accept_nonblock.py \
        -e net_hosted/accept_timeout.py \
        -e net_hosted/connect_nonblock_xfer.py \
        -e net_hosted/ssl_getpeercert.py \
        -e net_hosted/uasyncio_start_server.py \
    | tee -a ${resultsFile}

  echo
  echo "  done."
  echo

fi


if [ ${implemented} -eq 1 ]; then


  echo "  running implemented tests ..."
  echo

  ./run-tests.py --target psoc6 --device ${device} \
          io/builtin_print_file.py \
    | tee -a ${resultsFile}

  echo

  ./run-tests.py --target psoc6 --device ${device} \
        -d \
          basics \
          extmod \
          feature_check \
          float \
          frozen \
          import \
          inlineasm \
          micropython \
          misc \
          perf_bench \
          psoc6 \
          stress \
          unicode \
        \
        -e basics/async_for \
        -e basics/builtin_pow3_intbig.py \
        -e basics/builtin_print.py \
        -e basics/fun_largestate.py \
        -e basics/list_compare.py \
        -e basics/tuple_compare.py \
        -e basics/unpack1.py \
        \
        -e extmod/re_stack_overflow.py \
        -e extmod/socket_udp_nonblock.py \
        -e extmod/vfs_lfs_mtime.py \
        \
        -e feature_check/async_check.py \
        -e feature_check/bytearray.py \
        -e feature_check/byteorder.py \
        -e feature_check/complex.py \
        -e feature_check/const.py \
        -e feature_check/coverage.py \
        -e feature_check/float.py \
        -e feature_check/io_module.py \
        -e feature_check/native_check.py \
        -e feature_check/repl_emacs_check.py \
        -e feature_check/repl_words_move_check.py \
        -e feature_check/set_check.py \
        -e feature_check/slice.py \
        \
        -e float/math_domain_special.py \
        \
        -e import/builtin_ext.py \
        -e import/gen_context.py \
        -e import/import1a.py \
        -e import/import2a.py \
        -e import/import3a.py \
        -e import/import_broken.py \
        -e import/import_circular.py \
        -e import/import_file.py \
        -e import/import_long_dyn.py \
        -e import/import_override.py \
        -e import/import_pkg1.py \
        -e import/import_pkg2.py \
        -e import/import_pkg3.py \
        -e import/import_pkg4.py \
        -e import/import_pkg5.py \
        -e import/import_pkg6.py \
        -e import/import_pkg7.py \
        -e import/import_pkg8.py \
        -e import/import_pkg9.py \
        -e import/module_dict.py \
        -e import/try_module.py \
        \
        -e micropython/extreme_exc.py \
        -e micropython/memstats.py \
        \
        -e perf_bench/core_import_mpy_multi.py \
        -e perf_bench/core_import_mpy_single.py \
        -e 'perf_bench/viper_call.*.py' \
        \
        -e 'stress/recursi.*.py' \
        \
        -e unicode/file1.py \
        -e unicode/file2.py \
        -e unicode/file_invalid.py \
    | tee -a ${resultsFile}
  
  echo
  echo "  done."
  echo

fi


### not yet enabled/implemented, therefore failing
if [ ${notYetImplemented} -eq 1 ]; then

  echo "  running not yet implemented tests ..."
  echo

  ./run-tests.py --target psoc6 --device ${device} -d \
        multi_bluetooth \
        multi_net \
        thread \
    | tee -a ${resultsFile}

  echo
  echo "  done."
  echo

fi


### all excluded because it is know that these tests fail. In case of folder 
#
#             - cpydiff : please refer to documentetion within tests
#             - cmdline : tests are for command line Python execution and not for embedded MPY
#             - internal_bench : used for measuring run time of certain operations. Will always fail because runtime of Python and MPY will differ.
#             - io      : tests required data files on board flash and a change in the file paths to match the flash mount point
#             - jni     : tests to be run in environment with JAVA available, ie Unix, Windows, ...

# 
### therefore have been placed in this category
###
if [ ${failing} -eq 1 ]; then

  echo "  running failing tests ..."
  echo


  # prepare execution of tests by uploading required files to on-board flash
  ../tools/mpremote/mpremote.py cp internal_bench/bench.py :/flash/bench.py


  ./run-tests.py --target psoc6 --device ${device} -d \
        cmdline \
        cpydiff \
        internal_bench \
        io \
        jni \
    | tee -a ${resultsFile}

  echo
  echo "  done."
  echo

fi


echo
echo "generating pass, skip and fail files ..."

grep -i 'pass ' ${resultsFile} > ${passResultsFile}
grep -i 'skip ' ${resultsFile} > ${skipResultsFile}
grep -i 'FAIL ' ${resultsFile} > ${failResultsFile}

echo "generating pass, skip and fail files done."


echo
echo "executing $0 $* done."

