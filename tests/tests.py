import requests
import concurrent.futures

def send_request(url):
    response = requests.get(url)
    print(response.status_code)

urls = ['http://localhost:8080'] * 10

with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
    executor.map(send_request, urls)