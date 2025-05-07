const starsContainer = document.getElementById('stars');
const ratingText = document.getElementById('ratingText');
let selectedRating = 0;

function renderStars(hoveredRating = 0) {
  starsContainer.innerHTML = ''; // clear current stars

  for (let i = 1; i <= 5; i++) {
    const star = document.createElement('span');
    star.classList.add('star');
    star.innerText = '★';

    // fill if hovered or selected
    if (i <= hoveredRating || i <= selectedRating) {
      star.classList.add('filled');
    }

    // mouse interactions
    star.addEventListener('mouseover', () => renderStars(i));
    star.addEventListener('mouseout', () => renderStars());
    star.addEventListener('click', () => {
      selectedRating = i;
      console.log(selectedRating)
      ratingText.innerText = `You rated: ${selectedRating} star${selectedRating > 1 ? 's' : ''}`;
    });

    starsContainer.appendChild(star);
  }
}

// initial render
renderStars();
