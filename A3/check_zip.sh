#!/bin/bash

echo "Checking zip file...."

if [ "$#" -ne 1 ]; then
    echo "You should provide your zip file as a parameter and nothing else!"
    echo "Usage: ./check_zip.sh [zip_file]"
    echo "Example Usage: ./check_zip.sh ./A0123456X.zip"
    exit 1
fi

if ! [[ $1 =~ ^A[0-9]{7}[A-Z].zip$ || $1 =~ ^A[0-9]{7}[A-Z]_A[0-9]{7}[A-Z].zip$ ]]; then
    echo -n "Your zip file is wrongly named: it should be <YourMatricNum>.zip if "
    echo -n "you're working alone or <YourMatricNum>_<PartnerMatricNum>.zip "
    echo "if you're working with a partner"
    exit 1
fi

echo "Unzipping file: $1"
if [ ! -f $1 ]; then
    echo "Zip file $1 does not exist."
    exit 1
fi

# Create temp folder for unzipping
tmp_folder="test_grading_aeN332Hp"
rm -rf $tmp_folder
mkdir $tmp_folder

cp $1 $tmp_folder
cd $tmp_folder
unzip $1 > /dev/null

# Check for PDF report
SINGLE_REPORT_COUNT=$(ls | grep -cE "^A[0-9]{7}[A-Z]_report.pdf$")
PARTNER_REPORT_COUNT=$(ls | grep -cE "^A[0-9]{7}[A-Z]_A[0-9]{7}[A-Z]_report.pdf$")
if [ $SINGLE_REPORT_COUNT -eq 0 ] && [ $PARTNER_REPORT_COUNT -eq 0 ]; then
    echo -n "PDF report is missing: your pdf report should be named "
    echo -n "<YourMatriNum>_report.pdf if you're working alone, or "
    echo -n "<YourMatricNum>_<PartnerMatricNum>_report.pdf if you're working " 
    echo "with a partner"
    exit 1
fi

# Check for files and directories
directories=(testcases)
files=("main.c" "Makefile")
files_to_be_replaced=("tasks.c" "tasks.h" "hostfile")
for dir in "${directories[@]}"; do
    if [ ! -d $dir ]
    then
        echo "Failed - $dir folder missing"
        exit 1
    fi
done

for file in "${files[@]}"; do
    if [ ! -f $file ]
    then
        echo "Failed - $file file missing"
        exit 1
    fi
done

for file in "${files_to_be_replaced[@]}"; do
    if [ -f $file ]
    then
        echo "Failed - $file detected in submission zip file."
        echo "-------- $file file will be replaced during grading."
        echo "-------- Please remove this file and make sure that your "
        echo "-------- submission does not depend on changes you made in $file."
        exit 1
    fi
done

# Download sample input file to check whether executables can be compiled and run
INPUT_FILE_ID="1P5KT471itpkR46JiWatSSBOqGkhU_oxY"
INPUT_FILE_NAME="check_zip_a03_files.zip"
# Download cookie
curl -c ./cookie -s -L "https://drive.google.com/uc?export=download&id=${INPUT_FILE_ID}" &> /dev/null
# Download sample input file
curl -Lb ./cookie "https://drive.google.com/uc?export=download&confirm=`awk '/download/ {print $NF}' ./cookie`&id=${INPUT_FILE_ID}" -o ${INPUT_FILE_NAME} &> /dev/null

# Check that project can compile
unzip -d . ${INPUT_FILE_NAME} &>/dev/null
make build &> /dev/null
if [ $? -ne 0 ]
then 
    echo "Failed - project does not compile with 'make build'"
    exit 1
fi

# Check that compiled file is called `a03`
if [ ! -f "a03" ]
then
    echo "Failed - the executable should be named `a03`"
    echo "-------- A common mistake is to name it `a03.out.`"
    echo "-------- If that's the issue: there should be no `.out` suffix"
    exit 1
fi

# Check that `a03` can run`
mpirun -n 3 -hostfile hostfile ./a03 gdrive_sample_input_files 1 1 1 temp.out 1
if [ $? -ne 0 ]
then 
    echo "Failed - `a03` executable cannot run"
    exit 1
fi

cd ../
rm -rf $tmp_folder
echo "Success"
