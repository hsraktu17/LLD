const starContainer = document.getElementById('stars')
const ratingText = document.getElementById('ratingText')
let starRating = 0

function renderStar(rating = 0) {
  starContainer.innerHTML = ''
  for(let i = 1; i <= 5; i++) {
    const star = document.createElement('span');
    star.classList.add('star');
    star.innerText = '★';
    if(i <= rating || i <= starRating) {
      star.classList.add('filled')
    }
      
    
    star.addEventListener('mouseover', () => renderStar(i))
    star.addEventListener('mouseout', ()=> renderStar())
    star.addEventListener('click', () => {
      selectedRating = i
      ratingText.innerHTML = `your rated ${starRating} star${starRating > 1 ? 's' : ''}`
    })

    starContainer.appendChild(star)
  }
}

renderStar()
