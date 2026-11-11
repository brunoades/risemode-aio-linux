// SPDX-License-Identifier: GPL-2.0-only
/* Rise Mode Aura Ice aa88:8666 temperature display. */

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/hid.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define RISE_VENDOR_ID  0xaa88
#define RISE_PRODUCT_ID 0x8666

struct rise_aio {
	struct hid_device *hdev;
	struct mutex lock;
	u8 last_temp;
	bool has_temp;
};

static ssize_t display_temp_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct rise_aio *aio = hid_get_drvdata(to_hid_device(dev));
	ssize_t ret;

	mutex_lock(&aio->lock);
	ret = aio->has_temp ? sysfs_emit(buf, "%u\n", aio->last_temp) : -ENODATA;
	mutex_unlock(&aio->lock);
	return ret;
}

static ssize_t display_temp_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct rise_aio *aio = hid_get_drvdata(to_hid_device(dev));
	/* The leading zero is the unnumbered HID Report ID. usbhid removes it. */
	u8 report[9] = { 0, 0, 0x1c, 0, 0, 0x1c, 0, 0, 0 };
	unsigned int temp;
	int ret;

	ret = kstrtouint(buf, 10, &temp);
	if (ret)
		return ret;
	if (temp > 99)
		return -ERANGE;

	report[1] = temp;
	mutex_lock(&aio->lock);
	ret = hid_hw_output_report(aio->hdev, report, sizeof(report));
	if (ret == sizeof(report)) {
		aio->last_temp = temp;
		aio->has_temp = true;
	}
	mutex_unlock(&aio->lock);

	if (ret < 0)
		return ret;
	return ret == sizeof(report) ? count : -EIO;
}

static DEVICE_ATTR_RW(display_temp);

static int rise_aio_probe(struct hid_device *hdev,
			  const struct hid_device_id *id)
{
	struct rise_aio *aio;
	int ret;

	aio = kzalloc(sizeof(*aio), GFP_KERNEL);
	if (!aio)
		return -ENOMEM;
	aio->hdev = hdev;
	mutex_init(&aio->lock);
	hid_set_drvdata(hdev, aio);

	ret = hid_parse(hdev);
	if (ret)
		goto err_free;

	ret = hid_hw_start(hdev, HID_CONNECT_HIDRAW);
	if (ret)
		goto err_free;

	ret = device_create_file(&hdev->dev, &dev_attr_display_temp);
	if (ret)
		goto err_stop;

	hid_info(hdev, "Rise Mode temperature display ready\n");
	return 0;

err_stop:
	hid_hw_stop(hdev);
err_free:
	hid_set_drvdata(hdev, NULL);
	kfree(aio);
	return ret;
}

static void rise_aio_remove(struct hid_device *hdev)
{
	struct rise_aio *aio = hid_get_drvdata(hdev);

	device_remove_file(&hdev->dev, &dev_attr_display_temp);
	hid_hw_stop(hdev);
	hid_set_drvdata(hdev, NULL);
	kfree(aio);
}

static const struct hid_device_id rise_aio_devices[] = {
	{ HID_USB_DEVICE(RISE_VENDOR_ID, RISE_PRODUCT_ID) },
	{ }
};
MODULE_DEVICE_TABLE(hid, rise_aio_devices);

static struct hid_driver rise_aio_driver = {
	.name = "hid-risemode-aio",
	.id_table = rise_aio_devices,
	.probe = rise_aio_probe,
	.remove = rise_aio_remove,
};
module_hid_driver(rise_aio_driver);

MODULE_AUTHOR("OpenAI Codex");
MODULE_DESCRIPTION("Rise Mode Aura Ice USB temperature display");
MODULE_LICENSE("GPL");
