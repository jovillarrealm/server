import requests
import concurrent.futures

def send_request(url):
    response = requests.get(url)
    print(response.status_code)

def send_stress(url):
    count =0
    for letter in url:
        response = requests.get(url)
        print(response.status_code)
    print(f"Done: {count}")
    
urls = ['http://localhost:8080'] * 10

with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
    executor.map(send_request, urls)
    #print("And now a stress test")
    #executor.map(send_request)