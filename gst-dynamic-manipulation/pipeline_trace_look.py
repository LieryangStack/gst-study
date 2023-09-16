#! /usr/local/bin/python3
# -*- coding:UTF-8 -*-

# 主程序
import string


def del_line_with_buffers_address (lines, buffers_address):
    count = len (buffers_address)
    for buffer_address in buffers_address :
        for line in lines :
            if buffer_address in line :
                lines.remove(line)
        count -= 1
        print (count)


def main():
    pipeline_trace_log = open ("./pipeline_trace.log", "r")
    finalize_log = open ("./pipeline_finalize.log", "w")
    lines = pipeline_trace_log.readlines()
    buffers_address = [] # 空列表

    for line in lines :
        if "unref 1->0" in line :
            # 先找到"TRACE"的位置，从该位置查找"0x"
            trace_position = line.find ("TRACE")
            buffer_position = line.find ("0x", trace_position)
            if buffer_position != -1 :
                # 得到地址字符串
                buffer_address = line[buffer_position:buffer_position+14]
                buffers_address.append (buffer_address)


    del_line_with_buffers_address (lines, buffers_address)

    finalize_log.writelines(lines)
    pipeline_trace_log.close()
    finalize_log.close()

if __name__ == "__main__":
    main()
