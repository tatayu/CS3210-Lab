#!/bin/bash

echo "Checking zip file...."

if [ "$#" -ne 1 ]; then
    echo "You should provide your zip file as a parameter and nothing else!"
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
    echo "File $1 does not exist."
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
fi

# Check for files and directories
directories=(testcases)
files=("goi_threads.c" "goi_omp.c")
for dir in "${directories[@]}"; do
    if [ ! -d $dir ]
    then
        echo "Failed - $dir folder missing"
        continue
    fi
done

for file in "${files[@]}"; do
    if [ ! -f $file ]
    then
        echo "Failed - $file file missing"
        continue
    fi
done

# Check that project can compile
make build &> /dev/null
if [ $? -ne 0 ]
then 
    echo "Failed - project does not compile with 'make build'"
fi

# Download sample input file to check whether executables can run
INPUT_FILE_ID="1Rs9U47R4kY_DBqUea5mNUV6czF3IBjXj"
INPUT_FILE_NAME="check_zip_test_input.in"
# Download cookie
curl -c ./cookie -s -L "https://drive.google.com/uc?export=download&id=${INPUT_FILE_ID}" &> /dev/null
# Download sample input file
curl -Lb ./cookie "https://drive.google.com/uc?export=download&confirm=`awk '/download/ {print $NF}' ./cookie`&id=${INPUT_FILE_ID}" -o ${INPUT_FILE_NAME} &> /dev/null

# Check that `goi_omp` can run`
./goi_omp ./${INPUT_FILE_NAME} test_omp.out 1 &> /dev/null
if [ $? -ne 0 ]
then 
    echo "Failed - omp implementation cannot run"
fi

# Check that `goi_threads` can run
./goi_threads ./${INPUT_FILE_NAME} test_threads.out 1 &> /dev/null
if [ $? -ne 0 ]
then 
    echo "Failed - threads implementation cannot run"
fi

cd ../
rm -rf $tmp_folder
echo "Success"
