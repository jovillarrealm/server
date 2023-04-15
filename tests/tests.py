import requests
import concurrent.futures

def send_request(url):
    response = requests.get(url)
    print(response.status_code)
    print("Done")

def send_stress(url):
    count =0
    for letter in url:
        response = requests.get(url)
        print(response.status_code)
        if response.status_code == 200:
            count +=1
    print(f"Done: {count}")
port = 8080
urls = ['http://localhost:{port}'] * 10

with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
    executor.map(send_request, urls)
    print("And now a stress test")
    executor.map(send_stress, urls)