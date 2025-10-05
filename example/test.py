import requests

def test():
    url = "http://127.0.0.1:9999"
    res = requests.get(url)

if __name__ == "__main__":
    test()