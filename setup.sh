#!/bin/sh

mkdir wdir
mkdir wdir/storage
touch wdir/file_descriptions
touch wdir/welcome
touch wdir/userlist

echo "Welcome to the BBS Setup!" >> wdir/welcome
echo "user 1234 15" >> wdir/userlist

# Test file
touch wdir/storage/testfile.txt

printf 'testfile.txt\nuser\n1\nThis is a test file. You may not need it.\n:END:\n' >> wdir/file_descriptions

echo "Lorem ipsum dolor sit amet consectetur adipiscing elit. Quisque faucibus ex sapien vitae pellentesque sem placerat. In id cursus mi pretium tellus duis convallis. Tempus leo eu aenean sed diam urna tempor. Pulvinar vivamus fringilla lacus nec metus bibendum egestas. Iaculis massa nisl malesuada lacinia integer nunc posuere. \
Ut hendrerit semper vel class aptent taciti sociosqu. Ad litora torquent per \
conubia nostra inceptos himenaeos." >> wdir/storage/testfile.txt