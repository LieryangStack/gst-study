#! /usr/local/bin/python3
# -*- coding:UTF-8 -*-

# 主程序
import string

def main():

    count = 0
    # r = 只读 r+ = 读写
    # w = 打开一个文件只用于写入。如果该文件已存在则打开文件，并从开头开始编辑，即原有内容会被删除。如果该文件不存在，创建新文件。
    # w+ = 打开一个文件用于读写。如果该文件已存在则打开文件，并从开头开始编辑，即原有内容会被删除。如果该文件不存在，创建新文件。
    # 使用 with 关键字系统会自动调用 f.close() 方法， with 的作用等效于 try/finally 语句是一样的。
    with open ("./pipeline.log", "r") as pipeline_log :
        lines = pipeline_log.readlines()
    with open ("./pipeline_del_buffer.log", "w") as pipeline_trace_log :
        for number, line in enumerate (lines) :
            if "The buffer will be not be reused" not in line :
                pipeline_trace_log.write (line)    
                print (number)


if __name__ == "__main__":
    main()
