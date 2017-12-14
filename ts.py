#!/usr/bin/python
# -*- coding: utf-8 -*-

from tornado.ioloop import IOLoop
from client import client_listen
from da import da_listen
from frame import frame
import logging
logging.basicConfig()
import tornado.gen 


@tornado.gen.coroutine 
def test():
    yield tornado.gen.sleep(60)
    tornado.ioloop.IOLoop.instance().stop()
    exit(0)

def main():
    frame.frame_init_pool(1<<20, 1<<10, 1)

    da = da_listen()
    da.bind(6001, reuse_port=True)
    
    da.start(1)
    client = client_listen()
    client._started = True
    client.bind(6002, reuse_port=True)
#     future = test()
#     IOLoop.instance().add_future(future, lambda f:f.result())
    IOLoop.instance().start()

if __name__ == '__main__':
    main()
#     import cProfile
#  
#     cProfile.run("main()")
#     cProfile.run("main()", "result")
#      
#     import pstats
#     p = pstats.Stats("result")
#     p.strip_dirs().sort_stats(-1).print_stats()
#     p.strip_dirs().sort_stats("name").print_stats()
#     p.strip_dirs().sort_stats("cumulative").print_stats(3)
#     p.sort_stats('time', 'cum').print_stats(1, 'main')
#     p.print_callees("main")