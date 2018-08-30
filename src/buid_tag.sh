
find . |grep -E "\.cpp$|\.h$|\.c$"|grep -v "pjsip" >tags_list
ctags -L tags_list -f .tags
rm tags_list

