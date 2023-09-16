#! /usr/local/bin/python3
# -*- coding:UTF-8 -*-

# 主程序
import string

def main():
    string = "TRACE"
    count = 0
    # r = 只读 r+ = 读写
    # w = 打开一个文件只用于写入。如果该文件已存在则打开文件，并从开头开始编辑，即原有内容会被删除。如果该文件不存在，创建新文件。
    # w+ = 打开一个文件用于读写。如果该文件已存在则打开文件，并从开头开始编辑，即原有内容会被删除。如果该文件不存在，创建新文件。
    pipeline_log = open ("./pipeline.log", "r")
    pipeline_trace_log = open ("./pipeline_trace.log", "w")
    for line in pipeline_log.readlines():
        count += 1
        if string in line:
            pipeline_trace_log.write(line)
            print(count)
    pipeline_log.close()
    print(count)

if __name__ == "__main__":
    main()
