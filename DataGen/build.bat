call emcc datafile\game.cpp -o emcc/game.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc datafile\id.cpp -o emcc/id.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc datafile\metadata.cpp -o emcc/metadata.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc datafile\objectdata.cpp -o emcc/objectdata.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc datafile\slk.cpp -o emcc/slk.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc datafile\unitdata.cpp -o emcc/unitdata.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc datafile\westrings.cpp -o emcc/westrings.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc datafile\wtsdata.cpp -o emcc/wtsdata.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc rmpq\adpcm\adpcm.cpp -o emcc/adpcm.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc rmpq\archive.cpp -o emcc/archive.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc rmpq\common.cpp -o emcc/common.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc rmpq\compress.cpp -o emcc/compress.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc rmpq\huffman\huff.cpp -o emcc/huff.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc rmpq\locale.cpp -o emcc/locale.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc rmpq\pklib\crc32.c -o emcc/crc32.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc rmpq\pklib\explode.c -o emcc/explode.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc rmpq\pklib\implode.c -o emcc/implode.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc utils/json.cpp -o emcc/json.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc utils/utf8.cpp -o emcc/utf8.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc parse.cpp -o emcc/parse.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc search.cpp -o emcc/search.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc image\image.cpp -o emcc/image.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc image\imageblp.cpp -o emcc/imageblp.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc image\imageblp2.cpp -o emcc/imageblp2.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc image\imagedds.cpp -o emcc/imagedds.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc image\imagegif.cpp -o emcc/imagegif.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc image\imagejpg.cpp -o emcc/imagejpg.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc image\imagepng.cpp -o emcc/imagepng.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc image\imagetga.cpp -o emcc/imagetga.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcapimin.c -o emcc/jcapimin.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcapistd.c -o emcc/jcapistd.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jccoefct.c -o emcc/jccoefct.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jccolor.c -o emcc/jccolor.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcdctmgr.c -o emcc/jcdctmgr.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jchuff.c -o emcc/jchuff.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcinit.c -o emcc/jcinit.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcmainct.c -o emcc/jcmainct.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcmarker.c -o emcc/jcmarker.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcmaster.c -o emcc/jcmaster.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcomapi.c -o emcc/jcomapi.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcparam.c -o emcc/jcparam.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcphuff.c -o emcc/jcphuff.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcprepct.c -o emcc/jcprepct.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jcsample.c -o emcc/jcsample.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jctrans.c -o emcc/jctrans.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdapimin.c -o emcc/jdapimin.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdapistd.c -o emcc/jdapistd.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdatadst.c -o emcc/jdatadst.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdatasrc.c -o emcc/jdatasrc.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdcoefct.c -o emcc/jdcoefct.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdcolor.c -o emcc/jdcolor.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jddctmgr.c -o emcc/jddctmgr.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdhuff.c -o emcc/jdhuff.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdinput.c -o emcc/jdinput.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdmainct.c -o emcc/jdmainct.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdmarker.c -o emcc/jdmarker.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdmaster.c -o emcc/jdmaster.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdmerge.c -o emcc/jdmerge.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdphuff.c -o emcc/jdphuff.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdpostct.c -o emcc/jdpostct.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdsample.c -o emcc/jdsample.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jdtrans.c -o emcc/jdtrans.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jerror.c -o emcc/jerror.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jfdctflt.c -o emcc/jfdctflt.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jfdctfst.c -o emcc/jfdctfst.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jfdctint.c -o emcc/jfdctint.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jidctflt.c -o emcc/jidctflt.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jidctfst.c -o emcc/jidctfst.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jidctint.c -o emcc/jidctint.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jidctred.c -o emcc/jidctred.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jmemmgr.c -o emcc/jmemmgr.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jmemnobs.c -o emcc/jmemnobs.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jquant1.c -o emcc/jquant1.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jquant2.c -o emcc/jquant2.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jpeg\source\jutils.c -o emcc/jutils.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc jass.cpp -o emcc/jass.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc detect.cpp -o emcc/detect.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\adler32.c -o emcc/adler32.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\compress.c -o emcc/compress1.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\crc32.c -o emcc/crc321.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\deflate.c -o emcc/deflate.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\infback.c -o emcc/infback.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\inffast.c -o emcc/inffast.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\inflate.c -o emcc/inflate.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\inftrees.c -o emcc/inftrees.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\trees.c -o emcc/trees.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\uncompr.c -o emcc/uncompr.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc zlib\source\zutil.c -o emcc/zutil.bc -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc utils/checksum.cpp -o emcc/checksum.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc utils/common.cpp -o emcc/common1.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc utils/file.cpp -o emcc/file.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc utils/path.cpp -o emcc/path.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc utils/strlib.cpp -o emcc/strlib.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc hash.cpp -o emcc/hash.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc webmain.cpp -o emcc/webmain.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.
call emcc webarc.cpp -o emcc/webarc.bc --std=c++11 -O3 -DNO_SYSTEM -DZ_SOLO -I.

call emcc emcc/adler32.bc emcc/compress1.bc emcc/crc321.bc emcc/deflate.bc emcc/infback.bc emcc/inffast.bc emcc/inflate.bc emcc/inftrees.bc emcc/trees.bc emcc/uncompr.bc emcc/zutil.bc emcc/checksum.bc emcc/common1.bc emcc/file.bc emcc/path.bc emcc/strlib.bc emcc/hash.bc emcc/game.bc emcc/id.bc emcc/metadata.bc emcc/objectdata.bc emcc/slk.bc emcc/unitdata.bc emcc/westrings.bc emcc/wtsdata.bc emcc/adpcm.bc emcc/archive.bc emcc/common.bc emcc/compress.bc emcc/huff.bc emcc/locale.bc emcc/crc32.bc emcc/explode.bc emcc/implode.bc emcc/json.bc emcc/utf8.bc emcc/parse.bc emcc/search.bc emcc/webmain.bc -o MapParser.js -s EXPORT_NAME="MapParser" -O3 -s WASM=1 -s MODULARIZE=1 -s EXPORTED_FUNCTIONS="['_malloc', '_free']" --post-js ./module-post.js -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=134217728 -s DISABLE_EXCEPTION_CATCHING=0
call emcc emcc/adler32.bc emcc/compress1.bc emcc/crc321.bc emcc/deflate.bc emcc/infback.bc emcc/inffast.bc emcc/inflate.bc emcc/inftrees.bc emcc/trees.bc emcc/uncompr.bc emcc/zutil.bc emcc/checksum.bc emcc/common1.bc emcc/file.bc emcc/path.bc emcc/strlib.bc emcc/hash.bc emcc/webarc.bc emcc/image.bc emcc/imageblp.bc emcc/imageblp2.bc emcc/imagedds.bc emcc/imagegif.bc emcc/imagejpg.bc emcc/imagepng.bc emcc/imagetga.bc emcc/jcapimin.bc emcc/jcapistd.bc emcc/jccoefct.bc emcc/jccolor.bc emcc/jcdctmgr.bc emcc/jchuff.bc emcc/jcinit.bc emcc/jcmainct.bc emcc/jcmarker.bc emcc/jcmaster.bc emcc/jcomapi.bc emcc/jcparam.bc emcc/jcphuff.bc emcc/jcprepct.bc emcc/jcsample.bc emcc/jctrans.bc emcc/jdapimin.bc emcc/jdapistd.bc emcc/jdatadst.bc emcc/jdatasrc.bc emcc/jdcoefct.bc emcc/jdcolor.bc emcc/jddctmgr.bc emcc/jdhuff.bc emcc/jdinput.bc emcc/jdmainct.bc emcc/jdmarker.bc emcc/jdmaster.bc emcc/jdmerge.bc emcc/jdphuff.bc emcc/jdpostct.bc emcc/jdsample.bc emcc/jdtrans.bc emcc/jerror.bc emcc/jfdctflt.bc emcc/jfdctfst.bc emcc/jfdctint.bc emcc/jidctflt.bc emcc/jidctfst.bc emcc/jidctint.bc emcc/jidctred.bc emcc/jmemmgr.bc emcc/jmemnobs.bc emcc/jquant1.bc emcc/jquant2.bc emcc/jutils.bc emcc/jass.bc emcc/detect.bc emcc/common.bc -o ArchiveLoader.js -s EXPORT_NAME="ArchiveLoader" -O3 -s WASM=1 -s MODULARIZE=1 -s EXPORTED_FUNCTIONS="['_malloc', '_free']" --post-js ./module-post.js -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=33554432
