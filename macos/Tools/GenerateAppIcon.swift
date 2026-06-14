import AppKit
import Foundation

enum IconError: Error {
    case sourceMissing(URL)
    case bitmapUnavailable
    case contentBoundsUnavailable
    case pngEncodingFailed
}

func contentBounds(in bitmap: NSBitmapImageRep, whiteThreshold: CGFloat = 245 / 255) -> NSRect? {
    var minX = bitmap.pixelsWide
    var minY = bitmap.pixelsHigh
    var maxX = -1
    var maxY = -1

    for y in 0..<bitmap.pixelsHigh {
        for x in 0..<bitmap.pixelsWide {
            guard let color = bitmap.colorAt(x: x, y: y)?.usingColorSpace(.deviceRGB) else {
                continue
            }

            if color.redComponent < whiteThreshold
                || color.greenComponent < whiteThreshold
                || color.blueComponent < whiteThreshold {
                minX = min(minX, x)
                minY = min(minY, y)
                maxX = max(maxX, x)
                maxY = max(maxY, y)
            }
        }
    }

    guard maxX >= minX, maxY >= minY else {
        return nil
    }

    return NSRect(
        x: minX,
        y: minY,
        width: maxX - minX + 1,
        height: maxY - minY + 1
    )
}

func squareCrop(around bounds: NSRect, imageSize: NSSize) -> NSRect {
    let side = min(max(bounds.width, bounds.height), min(imageSize.width, imageSize.height))
    let x = min(max(bounds.midX - side / 2, 0), imageSize.width - side)
    let y = min(max(bounds.midY - side / 2, 0), imageSize.height - side)
    return NSRect(x: x, y: y, width: side, height: side)
}

func renderIcon(source: NSImage, crop: NSRect, size: Int) throws -> NSBitmapImageRep {
    guard let bitmap = NSBitmapImageRep(
        bitmapDataPlanes: nil,
        pixelsWide: size,
        pixelsHigh: size,
        bitsPerSample: 8,
        samplesPerPixel: 4,
        hasAlpha: true,
        isPlanar: false,
        colorSpaceName: .deviceRGB,
        bytesPerRow: 0,
        bitsPerPixel: 0
    ), let context = NSGraphicsContext(bitmapImageRep: bitmap) else {
        throw IconError.bitmapUnavailable
    }

    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = context
    defer { NSGraphicsContext.restoreGraphicsState() }

    context.imageInterpolation = .high

    let destination = NSRect(x: 0, y: 0, width: size, height: size)
    let clip = NSBezierPath(
        roundedRect: destination,
        xRadius: CGFloat(size) * 0.205,
        yRadius: CGFloat(size) * 0.205
    )
    clip.addClip()
    source.draw(
        in: destination,
        from: crop,
        operation: .copy,
        fraction: 1,
        respectFlipped: false,
        hints: [.interpolation: NSImageInterpolation.high]
    )

    return bitmap
}

func writePNG(_ bitmap: NSBitmapImageRep, to url: URL) throws {
    guard let png = bitmap.representation(using: .png, properties: [:]) else {
        throw IconError.pngEncodingFailed
    }
    try png.write(to: url)
}

let root = URL(fileURLWithPath: FileManager.default.currentDirectoryPath)
let sourceURL = root
    .appendingPathComponent("Assets")
    .appendingPathComponent("AppIconSource.png")
let iconset = root
    .appendingPathComponent("Assets")
    .appendingPathComponent("AppIcon.iconset")

guard let source = NSImage(contentsOf: sourceURL) else {
    throw IconError.sourceMissing(sourceURL)
}
guard let tiff = source.tiffRepresentation,
      let bitmap = NSBitmapImageRep(data: tiff) else {
    throw IconError.bitmapUnavailable
}
guard let bounds = contentBounds(in: bitmap) else {
    throw IconError.contentBoundsUnavailable
}

let crop = squareCrop(around: bounds, imageSize: source.size)

try? FileManager.default.removeItem(at: iconset)
try FileManager.default.createDirectory(at: iconset, withIntermediateDirectories: true)

let sizes: [(name: String, points: CGFloat, scale: CGFloat)] = [
    ("icon_16x16.png", 16, 1),
    ("icon_16x16@2x.png", 16, 2),
    ("icon_32x32.png", 32, 1),
    ("icon_32x32@2x.png", 32, 2),
    ("icon_128x128.png", 128, 1),
    ("icon_128x128@2x.png", 128, 2),
    ("icon_256x256.png", 256, 1),
    ("icon_256x256@2x.png", 256, 2),
    ("icon_512x512.png", 512, 1),
    ("icon_512x512@2x.png", 512, 2)
]

for item in sizes {
    let pixelSize = Int(item.points * item.scale)
    let bitmap = try renderIcon(source: source, crop: crop, size: pixelSize)
    try writePNG(bitmap, to: iconset.appendingPathComponent(item.name))
}
