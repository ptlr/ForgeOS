#! /bin/bash
if test -z $1;
then
echo "输入备份名称："
read backupDir
backupDir="./backup/$backupDir"
else
backupDir="./backup/$1"
fi
echo "1、创建$backupDir"
mkdir $backupDir/
echo "2、备份makefile:"
cp makefile $backupDir/
echo "3、备份scrpts:"
cp -r scripts $backupDir/
echo "4、备份源代码:"
cp -r src $backupDir/