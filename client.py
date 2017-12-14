
import tornado.gen 
from tornado.tcpserver import TCPServer
from frame import frame

class client(frame):
    def __init__(self, stream, address):
        frame.__init__(self)
        self.stream = stream 
        self.addr = address
        
    def __del__(self):
        self.stream.close()
        
    @tornado.gen.coroutine 
    def get_frame(self, ip, port):
        count = 10 
        
        while count :
            count = count - 1 
            
            ret = self.get_frame_header(ip, port)
            
            if ret == frame.RET_OK:
                return 
            
            yield tornado.gen.sleep(1)
        raise Exception('client get frame header error.')
    
    @tornado.gen.coroutine 
    def read_frame(self, ip, port):
        
        try:
            count = 100
            yield self.get_frame(ip, port)
            
            while count:
                status, ds = self.frame_pop()
                if status == frame.RET_OK:
                    count = 100
                    yield self.stream.write(ds)
                    continue
                else:
                    count = count - 1 
                yield tornado.gen.sleep(0.1)
        except Exception as e:
            print(e)
        finally:
            self.put_frame_header()
                
    
    @tornado.gen.coroutine 
    def work(self):
        yield self.stream.read_bytes(10)
        yield self.read_frame(10, 10)
        
    

class client_listen(TCPServer):
    def __init__(self):
        TCPServer.__init__(self)
    
    def handle_stream(self, stream, address):
        loop = tornado.ioloop.IOLoop.instance()
        cli = client(stream, address)
        future = cli.work()
        
        loop.add_future(future, lambda f:f.result())
        
