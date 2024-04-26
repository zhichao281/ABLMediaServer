
window.runDetect = (model, video, canvas) => {

    console.time('runDetect');
    // detect objects in the image.
    model.detect(video).then(predictions => {
        console.timeEnd('runDetect');
        console.log('Predictions: ', predictions);

        const context = canvas.getContext('2d');
        context.clearRect(0, 0, canvas.width, canvas.height);
        context.font = '10px Arial';

        console.log('number of detections: ', predictions.length);
        for (let i = 0; i < predictions.length; i++) {
            context.beginPath();
            context.rect(...predictions[i].bbox);
            context.lineWidth = 2;
            context.strokeStyle = 'green';
            context.fillStyle = 'green';
            context.stroke();
            context.fillText(
                predictions[i].score.toFixed(3) + ' ' + predictions[i].class, predictions[i].bbox[0],
                predictions[i].bbox[1] > 10 ? predictions[i].bbox[1] - 5 : 10);
        }

        window.setTimeout(() => { 
            if (model.run) {
                model.run(model, video, canvas); 
             }
        }, 120);
    });
}

window.runPosenet = (model, video, canvas) => {

    console.time('runPosenet');
    model.estimatePoses(video, { decodingMethod: 'multi-person', maxDetections: 5 }).then(poses => {
        console.timeEnd('runPosenet');
        console.log('Predictions: ', poses);

        const ctx = canvas.getContext('2d');
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.font = '10px Arial';

        poses.forEach(({ score, keypoints }) => {
            console.log('keypoints: ', keypoints);
            for (let i = 0; i < keypoints.length; i++) {
                const keypoint = keypoints[i];
                if (keypoint.score >= 0.5) {
                    const { y, x } = keypoint.position;
                    console.log(keypoint.part, ' : ', x, "x", y);
                    ctx.beginPath();
                    ctx.arc(x, y, 3, 0, 2 * Math.PI);
                    ctx.fillStyle = 'red';
                    ctx.fill();
                }
            }
            const adjacentKeyPoints = posenet.getAdjacentKeyPoints(keypoints, 0.5);
            console.log('adjacentKeyPoints: ', adjacentKeyPoints);

            adjacentKeyPoints.forEach((keypoints) => {
                const org = keypoints[0].position;
                const dest = keypoints[1].position;
                ctx.beginPath();
                ctx.moveTo(org.x,org.y);
                ctx.lineTo(dest.x,dest.y);
                ctx.lineWidth = 2;
                ctx.strokeStyle = 'green';
                ctx.stroke();
            });

        });

        window.setTimeout(() => { 
            if (model.run) {
                model.run(model, video, canvas); 
             }
        }, 120);
    });
}

window.runDeeplab = (model, video, canvas) => {

    console.time('runDeeplab');
    model.segment(video).then(deeplabOutput => {
        console.timeEnd('runDeeplab');
        console.log('deeplabOutput: ', deeplabOutput);
        const { legend, height, width, segmentationMap } = deeplabOutput;

        const scalecanvas = document.createElement('canvas')
        scalecanvas.width = width;
        scalecanvas.height = height;
        const segmentationMapData = new ImageData(segmentationMap, width, height)
        var imageData = segmentationMapData.data;
        for (var i = 0; i < imageData.length; i += 4) {
            if ((imageData[i] == 0) && (imageData[i + 1] == 0) && (imageData[i + 2] == 0) && (imageData[i + 3] == 255)) {
                imageData[i + 3] = 0;
            } else {
                imageData[i + 3] = 200;
            }
        }
        scalecanvas.getContext('2d').putImageData(segmentationMapData, 0, 0);

        const ctx = canvas.getContext('2d');
        ctx.font = '16px Arial';
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.drawImage(scalecanvas, 0, 0, width, height, 0, 0, canvas.width, canvas.height);

        let cnt = 0
        Object.keys(legend).forEach((label) => {
            const [red, green, blue] = legend[label];

            ctx.fillStyle = `rgb(${red}, ${green}, ${blue})`;
            ctx.fillRect(0, cnt * 16 + 16, 32, 16);
            ctx.fillText(label, 40, cnt * 16 + 32);
            cnt++
        });

        window.setTimeout(() => { 
            if (model.run) {
                model.run(model, video, canvas); 
             }
        }, 120);
    });
}


window.runbodyPix = (model, video, canvas) => {

    console.time('runbodyPix');
    model.segmentMultiPersonParts(video).then(multiPersonPartSegmentation => {
        console.timeEnd('runbodyPix');

        console.log('multiPersonPartSegmentation: ', multiPersonPartSegmentation);
        const coloredPartImageData = bodyPix.toColoredPartMask(multiPersonPartSegmentation);

        const ctx = canvas.getContext('2d');
        if (coloredPartImageData) {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            bodyPix.drawMask(canvas, video, coloredPartImageData, 0.3);        
        }

        window.setTimeout(() => { 
            if (model.run) {
                model.run(model, video, canvas); 
             }
        }, 120);
    });
}

window.runblazeface = (model, video, canvas) => {

    console.time('runblazeface');
    model.estimateFaces(video, false).then(predictions => {
        console.timeEnd('runblazeface');

        console.log('predictions: ', predictions);

        const ctx = canvas.getContext('2d');
        context.clearRect(0, 0, canvas.width, canvas.height);
        for (let i = 0; i < predictions.length; i++) {
            const start = predictions[i].topLeft;
            const end = predictions[i].bottomRight;
            const size = [end[0] - start[0], end[1] - start[1]];
      
            ctx.rect(start[0], start[1], size[0], size[1]);
        }

        window.setTimeout(() => { 
            if (model.run) {
                model.run(model, video, canvas); 
             }
        }, 120);
    });
}
