import numpy as np
import colorsys
import PIL.Image as Image
import PIL.ImageDraw as ImageDraw
import PIL.ImageFont as ImageFont
from scipy import interpolate
import traceback

import cv2

CM_CONTRAST = "Contrast"
CM_IMPAINTING = "Impainting"
CM_NONE = "None"


def pad_pil_image(pil_img, aspect_ratio):
    img_array = np.array(pil_img)
    h, w, _ = np.shape(img_array)
    if h * 1.0 / w > aspect_ratio:
        hor_add = int(h * 1.0 / aspect_ratio - w) // 2
        ver_add = 0
    else:
        hor_add = 0
        ver_add = int(w * aspect_ratio - h) // 2
    new_image = cv2.copyMakeBorder(
        img_array,
        ver_add,
        ver_add,
        hor_add,
        hor_add,
        borderType=cv2.BORDER_REPLICATE
    )
    return Image.fromarray(new_image)


class CaptchaRenderer(object):
    def __init__(self, font_name, curve_dx, color_dist, noise_mul, aspect_ratio, vertical_squeeze, color_mode, shade_val=0.3):
        self.font_name = font_name
        self.curve_dx = curve_dx
        self.color_dist = color_dist
        self.noise_mul = noise_mul
        self.aspect_ratio = aspect_ratio
        assert color_mode in [CM_CONTRAST, CM_IMPAINTING, CM_NONE]
        self.color_mode = color_mode
        self.font_size = 30
        self.font_size_to_curve_coef = 1.0
        self.vertical_squeeze_val = vertical_squeeze
        self.reversed_squeeze = False
        if vertical_squeeze > 1:
            self.vertical_squeeze_val = 1.0 / vertical_squeeze
            self.reversed_squeeze = True
        self.font = ImageFont.truetype(self.font_name, self.font_size)
        self._image_h, self._image_w = 700, 2000
        bg_colour = (255, 255, 255, 0)
        self.bg_image = np.dot(
            np.ones(
                (self._image_h, self._image_w, 4), dtype='uint8'), np.diag(
                np.asarray(
                    (bg_colour), dtype='uint8')))
        self.shade_multiplier = shade_val

    def _get_curve(self):
        steps_per_point = self.font_size * self.font_size_to_curve_coef
        num_points = int(self._image_w // steps_per_point) + 1
        val = 0
        ys = []
        xs = []
        for i in range(num_points):
            xs.append(i * steps_per_point)
            ys.append(val)
            val += np.sqrt(steps_per_point) * self.curve_dx * (np.random.random() - 0.5) * 2
        tck = interpolate.splrep(xs, ys)
        x = np.arange(0, self._image_w)
        return interpolate.splev(x, tck)

    def _render_text(self, text, color):
        text_image = Image.fromarray(self.bg_image)
        draw = ImageDraw.Draw(text_image)
        draw.text((100, (self._image_h - self.font_size) // 2), text,
                  font=self.font, fill=(color[0], color[1], color[2], 255))
        return np.array(text_image)

    def _curve_text_image(self, text_image):
        curve = self._get_curve()
        mesh = np.stack(np.meshgrid(np.arange(self._image_w), np.arange(self._image_h)), axis=-1).astype(np.float32)
        mesh[..., 1] += curve[None]
        remaped = cv2.remap(text_image, mesh, None, cv2.INTER_LINEAR, cv2.BORDER_REPLICATE, 1)
        return remaped

    def curved_text_image(self, text, color):
        text_image = self._render_text(text, color)
        return self._curve_text_image(text_image)

    def _get_borders(self, image, no_borders=False):
        x_min = np.min(image[..., 0], axis=0)
        y_min = np.min(image[..., 0], axis=1)
        left = -1
        right = len(x_min)
        for i in range(len(x_min)):
            if x_min[i] != 255 and left == -1:
                left = i
            if x_min[i] != 255:
                right = i
        top = -1
        bottom = len(y_min)
        for i in range(len(y_min)):
            if y_min[i] != 255 and top == -1:
                top = i
            if y_min[i] != 255:
                bottom = i
        if no_borders:
            return top, bottom, left, right
        return top - 3, bottom + 3, left - 3, right + 3

    def _circle_text(self, image_np, angle0, reversed_arc=False):
        t, b, l, r = self._get_borders(image_np, True)
        height = b - t
        if reversed_arc:
            radius0 = (r - l) / angle0 + height
        else:
            radius0 = (r - l) / angle0
        mesh = np.stack(np.meshgrid(np.arange(self._image_w), np.arange(self._image_h)), axis=-1).astype(np.float32)
        mesh[..., 0] -= self._image_w / 2
        if angle0 > np.pi:
            shift = radius0 + height
        else:
            shift = radius0
        if reversed_arc:
            mesh[..., 1] += np.cos(angle0 / 2) * shift - 30
            angle = np.arctan2(mesh[..., 0], mesh[..., 1])
        else:
            mesh[..., 1] -= 1 * self._image_h + np.cos(angle0 / 2) * shift - 30
            angle = np.arctan2(mesh[..., 0], -mesh[..., 1])
        rad = np.sqrt(mesh[..., 0] ** 2 + mesh[..., 1] ** 2)
        a_left = - angle0 / 2
        source_x = (angle - a_left) * (r - l) / angle0 + l
        if reversed_arc:
            source_y = (rad - radius0) + b
        else:
            source_y = -(rad - radius0) + b
        res = cv2.remap(image_np, source_x, source_y, cv2.INTER_LINEAR, cv2.BORDER_REPLICATE, 1)
        return res

    def load_bg(self, path):
        bg = Image.open(path, mode='r').convert("RGBA")
        return bg

    def check_borders(self, borders):
        top, bottom, left, right = borders
        if left < 1 or top < 1:
            return False
        if right > self._image_w - 2 or left > self._image_h - 2:
            return False
        if left >= right or top >= bottom:
            return False
        return True

    def check_is_bg_color_good(self, bg, text_color):
        colors = np.array(bg)[..., :3].reshape(-1, 3) / 255.
        text_light = colorsys.rgb_to_hsv(*(text_color[:3] / 255.))[2]
        if colors.shape[0] > 10000:
            colors = colors[np.random.randint(0, colors.shape[0], 10000)]
        hls = []
        for color in colors:
            hls.append(colorsys.rgb_to_hsv(*color))
        lights = np.array(hls)[..., 2]
        min_light = np.min(np.abs(lights - text_light))
        if min_light > 0.2:
            return True
        return False

    def _choose_text_color(self, bg):
        colors = np.array(bg)[..., :3].reshape(-1, 3) / 255.
        if colors.shape[0] > 10000:
            colors = colors[np.random.randint(0, colors.shape[0], 10000)]
        hls = []
        for color in colors:
            hls.append(colorsys.rgb_to_hsv(*color))
        hue, s, l = np.mean(np.array(hls), axis=0)
        if self.color_mode == CM_CONTRAST:
            if l < 0.5:
                l = 1.0 - np.random.rand() * 0.1
            else:
                l = np.random.rand() * 0.1
        else:
            l = np.random.rand() * 0.8 + 0.2
        s = np.random.rand() * 0.9 + 0.1

        hue_add = np.random.rand() * self.color_dist * 2 - self.color_dist
        if abs(hue_add) < self.color_dist:
            hue_add += self.color_dist * hue_add / abs(hue_add)
        hue += hue_add
        if hue > 1:
            hue -= 1
        if hue < 0:
            hue += 1
        return (np.array(colorsys.hsv_to_rgb(hue, s, l)) * 255).astype("uint8")

    def add_noise(self, image):
        row, col, ch = np.array(image).shape
        gauss = np.random.randn(row, col, ch) * self.noise_mul
        gauss = gauss.reshape(row, col, ch)
        noisy = (np.clip(np.array(image) / 255 + np.array(image) / 255 * gauss, 0, 1) * 255).astype("uint8")
        return noisy

    def _centralize_image(self, image_np):
        t, b, l, r = self._get_borders(image_np)
        height = b - t
        width = r - l
        left = self.font_size
        top = self._image_h // 2 - height // 2
        new_image = self.bg_image
        new_image[top:top + height, left:left + width] = image_np[t:b, l:r]
        return new_image

    def vertical_squeeze(self, image_np):
        if self.vertical_squeeze_val == 1.0:
            return image_np
        h, w, _ = image_np.shape
        mesh = np.stack(np.meshgrid(np.arange(w), np.arange(h)), axis=-1).astype(np.float32)
        new_sizes = np.arange(self.vertical_squeeze_val, 1, (1 - self.vertical_squeeze_val) / w)
        if self.reversed_squeeze:
            new_sizes = new_sizes[::-1]
        mesh[..., 1] /= new_sizes
        remaped = cv2.remap(image_np, mesh, None, cv2.INTER_LINEAR, cv2.BORDER_REPLICATE, 1)
        return remaped

    def impaint(self, bg, color):
        bg_hsv = np.array(bg.convert('HSV')).astype("float32")
        text_y = colorsys.rgb_to_hsv(*(color[:3] / 255.))[2]

        diff = (bg_hsv[..., 2] / 255. - text_y)
        imapint_threshold = self.color_dist
        pos_mask = ((diff < imapint_threshold) & (diff >= 0)).astype(np.float32)
        neg_mask = ((diff > -imapint_threshold) & (diff < 0)).astype(np.float32)
        bg_hsv[..., 2] += (imapint_threshold + 0.01 - diff) * pos_mask * 255.
        bg_hsv[..., 2] += (-imapint_threshold - 0.01 - diff) * neg_mask * 255.
        bg_hsv[..., 2] = bg_hsv[..., 2].clip(0, 255)
        return Image.fromarray(bg_hsv.astype("uint8"), mode='HSV').convert("RGBA")

    def shade(self, image):
        bg_hsv = np.array(image.convert('HSV')).astype("float32")
        shade = np.ones_like(bg_hsv)[..., 2]
        point1_x = np.random.randint(0, shade.shape[1])
        point1_y = 0 if np.random.rand() < 0.5 else shade.shape[0] - 1
        point2_x = shade.shape[0] // 2
        point2_y = np.random.randint(0, shade.shape[1])
        mesh = np.meshgrid(np.arange(0, shade.shape[1]), np.arange(0, shade.shape[0]))
        X0 = point1_y - point2_y
        Y0 = point2_x - point1_x
        Z = point1_x * point2_y - point2_x * point1_y
        mask_val = (X0 * mesh[1] + Y0 * mesh[0] + Z)
        mask = mask_val > 0 if np.random.rand() < 0.5 else mask_val < 0
        shade = shade * (1 - self.shade_multiplier) + self.shade_multiplier * mask.astype(np.float32)
        bg_hsv[..., 2] = bg_hsv[..., 2] * shade
        return Image.fromarray(bg_hsv.astype("uint8"), mode='HSV').convert("RGBA")

    def generate_image(self, text, bg_path, arc_angle, reversed_arc):
        try:
            bg = self.load_bg(bg_path)
            color = self._choose_text_color(bg)
            if arc_angle != 0:
                text_image = self._render_text(text, color)
                circled_text = self._circle_text(text_image, arc_angle, reversed_arc)
                text_image = self._centralize_image(circled_text)
                text_image = self._curve_text_image(text_image)
            else:
                text_image = self.curved_text_image(text, color)
            borders = self._get_borders(text_image)
            ok = self.check_borders(borders)
            if (not ok):
                return None, None, "bad borders"
            t, b, l, r = borders
            squeezed = self.vertical_squeeze(text_image[t:b + 1, l:r + 1])
            text_image_cropped = pad_pil_image(Image.fromarray(squeezed), self.aspect_ratio)
            w_cropped, h_cropped = text_image_cropped.width, text_image_cropped.height
            x_bg, y_bg = np.random.randint(0, bg.size[0] - w_cropped), np.random.randint(0, bg.size[1] - h_cropped)
            bg_croped = bg.crop((x_bg, y_bg, x_bg + w_cropped, y_bg + h_cropped))
            if self.color_mode == CM_IMPAINTING:
                bg_croped = self.impaint(bg_croped, color)
            color_good = self.check_is_bg_color_good(bg_croped, color)
            final_image = Image.alpha_composite(bg_croped, text_image_cropped)
            if self.shade_multiplier != 0:
                final_image = self.shade(final_image)
            return Image.fromarray(self.add_noise(final_image)).convert("RGB"), color_good, None
        except BaseException:
            return None, None, traceback.format_exc()


def main():
    cr = CaptchaRenderer("/home/iigc/captcha/Sin.ttf", 6.0, 0.3, 0.02, 0.9, CM_IMPAINTING, 0.0)
    image = cr.generate_image("test", "/home/iigc/captcha/99.jpg", np.pi, True)
    print(image[1], image[2])
    # image.save("img.jpg")


if __name__ == '__main__':
    main()
