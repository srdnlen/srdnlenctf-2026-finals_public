import time
from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from selenium.common.exceptions import TimeoutException, WebDriverException


def visit(flag, web_dom, css):
    try:
        options = Options()
        options.add_argument("--headless")
        options.add_argument("--no-sandbox")
        options.add_argument("--disable-dev-shm-usage")
        options.add_argument("--disable-gpu")
        options.add_argument("--enable-logging")
        options.add_argument("--v=1")
        options.page_load_strategy = "normal"

        driver = webdriver.Chrome(options=options)

        driver.set_page_load_timeout(120)

        try:
            driver.get(f"http://{web_dom}")
        except TimeoutException:
            print("[!] Initial load timeout")

        driver.add_cookie({
            "name": "secret",
            "value": flag,
            "path": "/",
        })

        try:
            driver.get(
                f"http://{web_dom}/?css={css}"
            )
        except TimeoutException:
            print("[!] CSS page timeout (expected)")

        time.sleep(30)

    except WebDriverException as e:
        print("[!] webdriver error:", e, flush=True)

    finally:
        driver.quit()