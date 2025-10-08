/** @type {import('tailwindcss').Config} */
module.exports = {
  content: [
    "./src/client/index.html",
    "./src/client/src/**/*.{ts,tsx}"
  ],
  theme: {
    extend: {
      colors: {
        midnight: "#0f172a"
      },
      boxShadow: {
        "brand-lg": "0 12px 30px rgba(56, 189, 248, 0.35)"
      }
    }
  },
  plugins: []
};
