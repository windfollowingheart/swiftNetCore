import socket

def send_tcp_request(host: str, port: int, message: str) -> str:
    """
    发送TCP请求并接收响应
    
    参数:
        host: 服务器主机名或IP地址
        port: 服务器端口号
        message: 要发送的消息
    
    返回:
        服务器响应的消息
    """
    # 创建TCP套接字
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # 连接服务器
        s.connect((host, port))
        
        # 发送消息（需编码为bytes）
        s.sendall(message.encode('utf-8'))
        
        # 接收服务器响应（最多1024字节）
        data = s.recv(1024)
    
    return data.decode('utf-8')

if __name__ == "__main__":
    # 服务器地址和端口
    server_host = 'localhost'
    server_port = 9999
    
    # 要发送的消息
    request_message = "Hello, server! This is a TCP test."
    
    # 发送请求并获取响应
    response = send_tcp_request(server_host, server_port, request_message)
    
    print(f"发送到服务器: {request_message}")
    print(f"从服务器接收: {response}")    