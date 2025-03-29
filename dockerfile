# Stage 1: Node.js stage for Tailwind CSS processing
FROM node:18 AS node-builder

WORKDIR /app
COPY . . 
RUN cd templates && npm install

RUN npm install tailwindcss @tailwindcss/cli
# Build the CSS
RUN pwd

RUN cd templates && npx tailwindcss -i style.css -o output.css

# Stage 2: Go build stage
FROM golang:latest

WORKDIR /app
COPY . .
COPY --from=node-builder /app/templates/output.css ./templates/

# Install dependencies
RUN go mod tidy

# Expose the application port
EXPOSE 3000

# Run the Go application
ENTRYPOINT ["go", "run", "main.go"]