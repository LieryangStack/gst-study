DOT_FILES_DIR="./"
PNG_FILES_DIR="./"

DOT_FILES=`ls $DOT_FILES_DIR | grep dot`

for dot_file in $DOT_FILES
do
  png_file=`echo $dot_file | sed s/.dot/.png/`
  dot -Tpng $dot_file > $png_file
done

