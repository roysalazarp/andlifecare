# Tailwind

A utility-first CSS framework packed with classes like `flex`, `pt-4`, `text-center`, and `rotate-90` that can be composed to build any design directly in your markup.

The binary `tailwindcss-v3.4.1-linux-x64` is a CLI tool that can be used to create CSS utility classes corresponding to those specified in the markup file.

```sh
# Example phseudo command:
# tailwindcli -i ./src/input.css -o ./src/output.css --watch

# Run from the project root directory
./tailwind/tailwindcss-v3.4.1-linux-x64 -i ./src/web/static/globals.css -o ./src/web/static/styles.css --watch
```

This project currently utilizes [Tailwind CLI v3.4.1](https://github.com/tailwindlabs/tailwindcss/releases/tag/v3.4.1)

Learn more about how to use tailwind [here](https://tailwindcss.com/).
