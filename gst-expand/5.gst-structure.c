#include<gst/gst.h>

gboolean
print_value(GQuark   field_id,
            const GValue * value,
            gpointer user_data){
  g_print("****foreach****\n");
  g_print("%d, %s\n",field_id,g_value_get_string(value));

  return TRUE;
}

int 
main(int argc, char *argv[]){

GValue a = G_VALUE_INIT;
GValue c = G_VALUE_INIT;
GstStructure *test_structure;

g_type_init();
g_value_init(&a, G_TYPE_STRING);
g_value_init(&c, G_TYPE_STRING);
g_assert(G_VALUE_HOLDS_STRING(&a));
g_value_set_string(&a, "Hello");
g_value_set_string(&c, "World");


/* Read GstStructure name and name_id */
test_structure = gst_structure_new_empty("lieryang");
g_print("%s\n",gst_structure_get_name (test_structure));
g_print("name_id = %d\n", gst_structure_get_name_id(test_structure));

/* 修改GstStructure的名字 */
gst_structure_set_name(test_structure, "EryangLi");
g_print("%s\n",gst_structure_get_name (test_structure));
g_print("name_id = %d\n", gst_structure_get_name_id(test_structure));

/* 设置成员信息 */
gst_structure_set_value(test_structure, "Li", &a);
gst_structure_set_value(test_structure, "L", &c);
gst_structure_set(test_structure, "Si", G_TYPE_STRING, "Apple",
                                  "Bo", G_TYPE_STRING, "1996", NULL);

/* 读取单个成员信息 */
const GValue *b = gst_structure_get_value(test_structure, "L");
g_print("%s\n", g_value_get_string(b));

/* 遍历成员信息 */
gst_structure_foreach(test_structure, print_value, NULL);


test_structure = gst_structure_new("Family", "boy", G_TYPE_STRING, "Li",
                                             "girl", G_TYPE_STRING, "Zheng",NULL);
gst_structure_foreach(test_structure, print_value, NULL);
}