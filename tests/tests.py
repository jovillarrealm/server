import requests
import concurrent.futures

def send_request(url):
    count = 0
    for letter in url:
        response = requests.get(url)
        print(response.status_code)
        count += 1
    print(f"Done: {count}")

urls = ['http://localhost:8080/example.html'] * 10

with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
    executor.map(send_request, urls)